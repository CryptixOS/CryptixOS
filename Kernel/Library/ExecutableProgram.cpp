/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/fcntl.h>
#include <Library/ExecutableProgram.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/VFS.hpp>

ErrorOr<void> ExecutableProgram::Load(PathView path, PageMap* pageMap,
                                      AddressSpace& addressSpace,
                                      Pointer       loadBase)
{
    m_LoadBase      = loadBase;
    auto maybeImage = LoadImage(path, pageMap, addressSpace);
    RetOnError(maybeImage);

    m_Image      = maybeImage.Value();
    m_EntryPoint = m_Image->EntryPoint();

    auto ldPath  = m_Image->InterpreterPath();
    if (ldPath.Empty()) return {};

    m_LoadBase = 0x41000000;
    maybeImage = LoadImage(ldPath, pageMap, addressSpace, true);
    RetOnError(maybeImage);

    m_Interpreter = maybeImage.Value();
    m_EntryPoint  = m_Interpreter->EntryPoint();

    return {};
}

Pointer ExecutableProgram::PrepareStack(Pointer            stackTopWritable,
                                        Pointer            stackTopVirt,
                                        Vector<StringView> argv,
                                        Vector<StringView> envp)
{
    auto                           stack = stackTopWritable.As<uintptr_t>();

    CPU::UserMemoryProtectionGuard guard;
    for (auto env : envp)
    {
        stack = Pointer(stack).Offset<uintptr_t*>(-env.Size() - 1);
        Memory::Copy(stack, env.Raw(), env.Size());
    }
    for (auto arg : argv)
    {
        stack = Pointer(stack).Offset<uintptr_t*>(-arg.Size() - 1);
        Memory::Copy(stack, arg.Raw(), arg.Size());
    }

    stack = Pointer(Math::AlignDown(Pointer(stack), 16));
    if ((argv.Size() + envp.Size() + 1) & 1) stack--;

    *(--stack) = 0;
    *(--stack) = 0;
    stack -= 2;
    stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eEntry),
    stack[1] = Image().EntryPoint();
    stack -= 2;
    stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eProgramHeader),
    stack[1] = Image().ProgramHeaderAddress();
    stack -= 2;
    stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eProgramHeaderEntrySize),
    stack[1] = Image().ProgramHeaderEntrySize();
    stack -= 2;
    stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eProgramHeaderCount),
    stack[1] = Image().ProgramHeaderCount();

    // if (m_InterpreterBase)
    // {
    //     stack -= 2;
    //     stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eInterpreterBase),
    //     stack[1] = m_InterpreterBase;
    // }
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::ePageSize),
    // stack[1] = PMM::PAGE_SIZE;
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eUid), stack[1] = 0;
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eEUid), stack[1] = 0;
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eGid), stack[1] = 0;
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eEGid), stack[1] = 0;
    // stack -= 2;
    // stack[0] = ToUnderlying(ELF::AuxiliaryValueType::eExec), stack[1] = 0;

    Pointer stackPointer = stackTopVirt;
    *(--stack)           = 0;
    stack -= envp.Size();
    for (usize i = 0; auto env : envp)
    {
        stackPointer -= env.Size() + 1;
        stack[i++] = stackPointer;
    }

    *(--stack) = 0;
    stack -= argv.Size();
    for (usize i = 0; auto arg : argv)
    {
        stackPointer -= arg.Size() + 1;
        stack[i++] = stackPointer;
    }

    *(--stack) = argv.Size();
    return stackTopVirt.Raw() - (stackTopWritable.Raw() - Pointer(stack).Raw());
}

ErrorOr<Ref<ELF::Image>>
ExecutableProgram::LoadImage(PathView path, PageMap* pageMap,
                             AddressSpace& addressSpace, bool interpreter)
{
    Ref entry = VFS::ResolvePath(VFS::RootDirectoryEntry().Raw(), path)
                    .Value()
                    .Entry;
    if (!entry) return Error(ENOENT);

    auto inode = entry->INode();
    if (!inode) return Error(ENOENT);

    auto maybeFile
        = VFS::Open(VFS::RootDirectoryEntry().Raw(), path, O_RDONLY, 0);
    RetOnError(maybeFile);

    Ref file  = maybeFile.Value();
    Ref image = CreateRef<ELF::Image>();

    if (!image->Load(file.Raw(), m_LoadBase)) return Error(ENOEXEC);

    auto forEachProgramHeader = [&](ELF::ProgramHeader* header) -> bool
    {
        if (header->Type == ELF::HeaderType::eLoad)
        {
            usize misalign  = header->VirtualAddress & (PMM::PAGE_SIZE - 1);
            usize pageCount = Math::DivRoundUp(
                header->SegmentSizeInMemory + misalign, PMM::PAGE_SIZE);

            Pointer phys = PMM::CallocatePages(pageCount);
            Assert(phys);

            auto  virt = header->VirtualAddress + m_LoadBase;
            usize size = pageCount * PMM::PAGE_SIZE;
            Assert(pageMap->MapRange(virt, phys, size,
                                     PageAttributes::eRWXU
                                         | PageAttributes::eWriteBack));
            auto region
                = new Region(phys, header->VirtualAddress + m_LoadBase, size);
            using VMM::Access;
            region->SetAccessMode(Access::eReadWriteExecute | Access::eUser);

            addressSpace.Insert(region->VirtualBase(), region);
            Memory::Copy(phys.Offset<Pointer>(misalign).ToHigherHalf(),
                         image->Raw().Offset(header->Offset),
                         header->SegmentSizeInFile);

            // if (interpreter)
            //     m_InterpreterBase = std::min(m_InterpreterBase, virt);
        }

        return true;
    };

    ELF::Image::ProgramHeaderEnumerator programHeaderIterator;
    programHeaderIterator.BindLambda(forEachProgramHeader);

    image->ForEachProgramHeader(programHeaderIterator);
    return image;
}
