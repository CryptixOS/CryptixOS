/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ELF.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"
#include "Utility/Math.hpp"

#include "VFS/INode.hpp"
#include "VFS/VFS.hpp"

namespace ELF
{
    uintptr_t Image::Load(std::string_view path)
    {
        INode* file = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
        Assert(file->Read(&header, 0, sizeof(header)) == sizeof(header));
        Assert(std::memcmp(&header.magic, MAGIC, 4) == 0);

        ProgramHeader current;
        for (usize i = 0; i < header.programEntryCount; i++)
        {
            Assert(file->Read(&current,
                              header.programHeaderTableOffset
                                  + i * header.programEntrySize,
                              sizeof(current))
                   == sizeof(current));
            if (current.type != HeaderType::eLoad) continue;

            usize pageCount   = current.segmentSizeInFile / PMM::PAGE_SIZE + 1;
            uintptr_t program = PMM::CallocatePages<uintptr_t>(pageCount);

            for (usize i = 0; i < pageCount; i++)
                VMM::GetKernelPageMap()->Map(
                    current.virtualAddress + i * PMM::PAGE_SIZE,
                    program + i * PMM::PAGE_SIZE,
                    PageAttributes::eRWXU | PageAttributes::eWriteBack);

            file->Read(reinterpret_cast<void*>(current.virtualAddress),
                       current.offset, current.segmentSizeInFile);
            LogInfo("Virtual Address: {:#x}, sizeInFile: {}, sizeInMemory: {}",
                    current.virtualAddress, current.segmentSizeInFile,
                    current.segmentSizeInMemory);
        }

        LogInfo("EntryPoint: {:#x}", header.entryPoint);
        return header.entryPoint;
    }
} // namespace ELF
