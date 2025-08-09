/*
 * Created by v1tr10l7 on 21.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Containers/Bitmap.hpp>

#include <VFS/Ext2Fs/Ext2Fs.hpp>
#include <VFS/Ext2Fs/Ext2FsAllocator.hpp>

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
        if (!blockBitmap.GetIndex(block))
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
            if (!bitmap.GetIndex(bit + (blockIndex * 8)))
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
