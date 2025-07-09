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

ErrorOr<void> ExecutableProgram::Load(PathView path, PageMap* pageMap,
                                      AddressSpace& addressSpace,
                                      Pointer       loadBase)
{
    Ref entry = VFS::ResolvePath(VFS::GetRootDirectoryEntry().Raw(), path)
                    .value()
                    .Entry;
    if (!entry) return Error(ENOENT);

    auto inode = entry->INode();
    if (!inode) return Error(ENOENT);

    auto maybeFile
        = VFS::Open(VFS::GetRootDirectoryEntry().Raw(), path, O_RDONLY, 0);
    RetOnError(maybeFile);

    // TODO(v1tr10l7): add refcounting to file descriptors

    // TODO(v1tr10l7): close the file
    auto file = maybeFile.value();
    if (!m_Image.Load(file, pageMap, addressSpace, loadBase))
        return Error(ENOEXEC);

    auto forEachProgramHeader = [&](ELF::ProgramHeader* header) -> bool
    {
        if (header->Type == ELF::HeaderType::eLoad)
        {
            usize misalign  = header->VirtualAddress & (PMM::PAGE_SIZE - 1);
            usize pageCount = Math::DivRoundUp(
                header->SegmentSizeInMemory + misalign, PMM::PAGE_SIZE);

            Pointer phys = PMM::CallocatePages(pageCount);
            Assert(phys);

            usize size = pageCount * PMM::PAGE_SIZE;
            Assert(pageMap->MapRange(
                header->VirtualAddress + loadBase.Raw(), phys, size,
                PageAttributes::eRWXU | PageAttributes::eWriteBack));
            auto region = new Region(
                phys, header->VirtualAddress + loadBase.Raw(), size);
            using VirtualMemoryManager::Access;
            region->SetProt(Access::eReadWriteExecute | Access::eUser,
                            PROT_READ | PROT_WRITE | PROT_EXEC);

            addressSpace.Insert(region->VirtualBase(), region);
            Memory::Copy(phys.Offset<Pointer>(misalign).ToHigherHalf(),
                         m_Image.Raw().Offset(header->Offset),
                         header->SegmentSizeInFile);
        }

        return true;
    };

    ELF::Image::ProgramHeaderEnumerator programHeaderIterator;
    programHeaderIterator.BindLambda(forEachProgramHeader);

    m_Image.ForEachProgramHeader(programHeaderIterator);

    delete file;
    return {};
}
