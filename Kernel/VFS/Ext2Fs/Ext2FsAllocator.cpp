/*
 * Created by v1tr10l7 on 21.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Containers/Bitmap.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.Ext2Fs.Ext2FsAllocator;

import :VFS.Ext2Fs.Ext2Fs;
import :VFS.Ext2Fs.Ext2FsAllocator;

void Ext2FsAllocator::Initialize(Ext2Fs* fs)
{
    m_Filesystem      = fs;
    m_Device          = fs->m_Device;

    auto superBlock   = m_Filesystem->GetSuperBlock();
    m_BlockSize       = m_Filesystem->GetBlockSize();
    m_BlockCount      = superBlock->BlockCount;
    m_BlockGroupCount = m_BlockCount / superBlock->BlocksPerGroup;
    m_BlocksPerGroup  = superBlock->BlocksPerGroup;
}

usize Ext2FsAllocator::AllocateINode()
{
    auto  superBlock = m_Filesystem->GetSuperBlock();
    usize blockCount = superBlock->BlockCount;
    if (superBlock->FreeINodeCount == 0) return 0;

    for (usize i = 0; i < blockCount; i++)
    {
        Ext2FsBlockGroupDescriptor blockGroup{};
        m_Filesystem->ReadBlockGroupDescriptor(&blockGroup, i);

        if (blockGroup.FreeBlockCount <= 0) continue;
        usize inode = AllocateINodeInGroup(i, blockGroup);
        if (!inode) continue;

        // TODO(v1tr10l7): Superblock Locking
        --blockGroup.FreeINodeCount;
        --superBlock->FreeINodeCount;
        m_Filesystem->WriteBlockGroupDescriptor(blockGroup, i);
        m_Device->Write(superBlock, 1024, sizeof(Ext2FsSuperBlock));

        return inode;
    }

    return 0;
}
void Ext2FsAllocator::FreeINode(ino_t ino)
{
    auto  superBlock = m_Filesystem->GetSuperBlock();
    usize blockSize  = m_Filesystem->GetBlockSize();
    --ino;

    usize blockGroupIndex                 = ino / superBlock->INodesPerGroup;
    Ext2FsBlockGroupDescriptor blockGroup = {};
    m_Filesystem->ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);

    Bitmap bitmap;
    bitmap.Allocate(superBlock->BlockCount);
    m_Device->Read(bitmap.Raw(), blockGroup.INodeUsageBitmapAddress * blockSize,
                   blockSize);
    bitmap.SetIndex(ino % superBlock->INodesPerGroup, false);

    m_Device->Write(bitmap.Raw(),
                    blockGroup.INodeUsageBitmapAddress * blockSize, blockSize);

    ++blockGroup.FreeINodeCount;
    ++superBlock->FreeINodeCount;

    m_Filesystem->WriteBlockGroupDescriptor(blockGroup, blockGroupIndex);
    m_Filesystem->FlushSuperBlock();
    bitmap.Free();
}

usize Ext2FsAllocator::AllocateBlock(Ext2FsINodeMeta& meta, u32 inode)
{
    auto blockGroupIndex = LocateFreeGroup();
    if (blockGroupIndex == 0) return 0;

    Ext2FsBlockGroupDescriptor blockGroup{};
    m_Filesystem->ReadBlockGroupDescriptor(&blockGroup, blockGroupIndex);
    Bitmap blockBitmap;
    blockBitmap.Allocate(m_BlockSize * 8);
    Memory::Fill(blockBitmap.Raw(), 0xff, m_BlockSize);

    m_Device->Read(blockBitmap.Raw(),
                   blockGroup.BlockUsageBitmapAddress * m_BlockSize,
                   m_BlockSize);

    usize foundBlock = 0;

    for (usize block = 0; block < m_BlockSize * 8; block++)
    {
        if (!blockBitmap.At(block))
        {
            blockBitmap.SetIndex(block, true);
            foundBlock = blockGroupIndex * m_BlocksPerGroup + block;
            break;
        }
    }
    if (!foundBlock)
    {
        // FIXME(v1tr10l7): Corrupted filesystem?
        // what should we do?
        blockGroup.FreeBlockCount = 0;
        goto exit;
    }

    m_Device->Write(blockBitmap.Raw(),
                    blockGroup.BlockUsageBitmapAddress * m_BlockSize,
                    m_BlockSize);
    m_Device->Read(blockBitmap.Raw(),
                   blockGroup.BlockUsageBitmapAddress * m_BlockSize,
                   m_BlockSize);
    --blockGroup.FreeBlockCount;

exit:
    m_Filesystem->WriteBlockGroupDescriptor(blockGroup, blockGroupIndex);
    PMM::FreePages(Pointer(blockBitmap.Raw()).FromHigherHalf(), 1);
    return foundBlock;
}

usize Ext2FsAllocator::LocateFreeGroup(usize blockCount)
{
    for (usize groupIndex = 1; groupIndex < m_BlockGroupCount; ++groupIndex)
    {
        Ext2FsBlockGroupDescriptor blockGroup{};
        m_Filesystem->ReadBlockGroupDescriptor(&blockGroup, groupIndex);

        if (blockGroup.FreeBlockCount > 0) return groupIndex;
    }

    return 0;
}
usize Ext2FsAllocator::AllocateINodeInGroup(
    usize blockGroupIndex, Ext2FsBlockGroupDescriptor& blockGroup)
{
    Bitmap bitmap;

    auto   superBlock = m_Filesystem->GetSuperBlock();
    usize  blockSize  = m_Filesystem->GetBlockSize();
    bitmap.Allocate(superBlock->BlockCount);
    m_Device->Read(bitmap.Raw(), blockGroup.INodeUsageBitmapAddress * blockSize,
                   blockSize);

    usize inode = 0;
    for (usize blockIndex = 0; blockIndex < blockSize; blockIndex++)
    {
        if (bitmap.Raw()[blockIndex] == 0xff) continue;

        for (usize bit = 0; bit < 8; bit++)
        {
            if (!bitmap.At(bit + (blockIndex * 8)))
            {
                inode = (blockGroupIndex * superBlock->INodesPerGroup)
                      + blockIndex * 8 + bit + 1;
                if ((inode > superBlock->FirstNonReservedINode && inode > 11))
                {
                    bitmap.SetIndex(bit + (blockIndex * 8), true);
                    break;
                }
            }
        }

        if (inode) break;
    }

    if (inode)
        m_Device->Write(bitmap.Raw(),
                        blockGroup.INodeUsageBitmapAddress * blockSize,
                        blockSize);

    return inode;
}
