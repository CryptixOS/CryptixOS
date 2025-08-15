/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/fcntl.h>
#include <Library/ELF/Loader.hpp>
#include <Library/ExecutableProgram.hpp>
#include <Library/StackBuilder.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/VFS.hpp>

ErrorOr<void> ExecutableProgram::Load(PathView path, PageMap* pageMap,
                                      AddressSpace& addressSpace,
                                      Pointer       loadBase)
{
    m_LoadBase   = loadBase;
    m_Image      = TryOrRet(LoadImage(path, pageMap, addressSpace));
    m_EntryPoint = m_Image->EntryPoint();

    auto ldPath  = m_Image->InterpreterPath();
    if (ldPath.Empty()) return {};

    m_LoadBase    = 0x41000000;
    m_Interpreter = TryOrRet(LoadImage(ldPath, pageMap, addressSpace, true));
    m_EntryPoint  = m_Interpreter->EntryPoint();

    return {};
}

Pointer ExecutableProgram::PrepareStack(Pointer            stackTopWritable,
                                        Pointer            stackTopVirt,
                                        Vector<StringView> argArr,
                                        Vector<StringView> envArr)
{
    StackBuilder                   builder(stackTopWritable);

    CPU::UserMemoryProtectionGuard guard;
    // --- 1. Copy envArr strings onto stack ---
    Pointer                        stackPhys = stackTopVirt;
    Vector<upointer>               envp;
    for (auto env : envArr)
    {
        stackPhys -= env.Size() + 1;
        envp.EmplaceBack(stackPhys);
        builder.Write(env.Raw(), env.Size() + 1);
    }

    // --- 2. Copy argArr strings onto stack ---
    Vector<upointer> argv;
    for (auto arg : argArr)
    {
        stackPhys -= arg.Size() + 1;
        argv.EmplaceBack(stackPhys);
        builder.Write(arg.Raw(), arg.Size() + 1);
    }

    // --- 3. Copy executable path for AT_EXECFN ---
    auto execPath = m_ExecutablePath;
    stackPhys -= execPath.Size() + 1;
    upointer execPathAddr = stackPhys;
    builder.Write(execPath.Raw(), execPath.Size() + 1);

    // --- 4. Align stack to 16 bytes ---
    builder.Align(16);
    // padding
    if ((argArr.Size() + envArr.Size() + 1) & 1) builder.Write(0);

    // --- 5. Push null terminators for argArr/envArr ---
    builder.Write(0);
    builder.Write(0);

    using AuxVal = ELF::AuxiliaryValueType;
    // --- 6. Push auxv entries ---
    builder.Write(AuxVal::eEntry, Image().EntryPoint());
    builder.Write(AuxVal::eProgramHeaders, Image().ProgramHeaderAddress());
    builder.Write(AuxVal::eProgramHeaderEntrySize,
                  Image().ProgramHeaderEntrySize());
    builder.Write(AuxVal::eProgramHeaderCount, Image().ProgramHeaderCount());

    if (m_InterpreterBase)
        builder.Write(AuxVal::eInterpreterBase, m_InterpreterBase);
    builder.Write(AuxVal::ePageSize, PMM::PAGE_SIZE);

    Credentials creds{};
    builder.Write(AuxVal::eUserID, creds.UserID);
    builder.Write(AuxVal::eEffectiveUserID, creds.EffectiveUserID);
    builder.Write(AuxVal::eGroupID, creds.GroupID);
    builder.Write(AuxVal::eEffectiveGroupID, creds.EffectiveGroupID);
    builder.Write(AuxVal::eExecutablePath, execPathAddr);

    // --- 7. Push envArr pointers ---
    builder.Write(0);
    for (usize i = envp.Size(); i > 0; i--) builder.Write(envp[i - 1]);

    // --- 8. Push argArr pointers ---
    builder.Write(0);
    for (usize i = argv.Size(); i > 0; i--) builder.Write(argv[i - 1]);

    // --- 9. Push argc ---
    builder.Write(argArr.Size());

    // --- 10. Return new stack pointer ---
    return stackTopVirt - (builder.Top() - builder.Current());
}

ErrorOr<Ref<ELF::Image>>
ExecutableProgram::LoadImage(PathView path, PageMap* pageMap,
                             AddressSpace& addressSpace, bool interpreter)
{
    Ref entry
        = VFS::ResolvePath(VFS::RootDirectoryEntry().Raw(), path).Value().Entry;
    if (!entry) return Error(ENOENT);

    auto inode = entry->INode();
    if (!inode) return Error(ENOENT);

    auto file = TryOrRet(
        VFS::Open(VFS::RootDirectoryEntry().Raw(), path, O_RDONLY, 0));
    Ref image = CreateRef<ELF::Image>();

    if (!image->Load(file.Raw(), m_LoadBase)) return Error(ENOEXEC);

    Pointer minVirt = 0;
    for (usize i = 0; i < image->ProgramHeaderCount(); i++)
    {
        auto& header = image->ProgramHeader(i);

        if (header.Type == ELF::HeaderType::eLoad
            /*&& image->InterpreterPath().Empty()*/)
        {
            usize misalign  = header.VirtualAddress & (PMM::PAGE_SIZE - 1);
            usize pageCount = Math::DivRoundUp(
                header.SegmentSizeInMemory + misalign, PMM::PAGE_SIZE);

            Pointer phys = PMM::CallocatePages(pageCount);
            Assert(phys);

            auto  virt = header.VirtualAddress + m_LoadBase;
            usize size = pageCount * PMM::PAGE_SIZE;
            Assert(pageMap->MapRange(virt, phys, size,
                                     PageAttributes::eRWXU
                                         | PageAttributes::eWriteBack));
            auto region
                = new Region(phys, header.VirtualAddress + m_LoadBase, size);
            using VMM::Access;
            region->SetAccessMode(Access::eReadWriteExecute | Access::eUser);

            addressSpace.Insert(region->VirtualBase(), region);
            Memory::Copy(phys.Offset<Pointer>(misalign).ToHigherHalf(),
                         image->Raw().Offset(header.Offset),
                         header.SegmentSizeInFile);

            minVirt = Min(minVirt.Raw(), virt);
        }
    }

    if (interpreter) m_InterpreterBase = minVirt;
    return image;
}
