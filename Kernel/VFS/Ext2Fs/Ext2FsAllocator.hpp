/*
 * Created by v1tr10l7 on 21.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Ext2Fs/Ext2FsStructures.hpp>

class Ext2Fs;
class Ext2FsAllocator
{
  public:
    Ext2FsAllocator() = default;
    void  Initialize(Ext2Fs* fs);

    usize AllocateINode();
    usize AllocateBlock(Ext2FsINodeMeta& meta, u32 inode);

  private:
    Ext2Fs*      m_Filesystem      = nullptr;
    class INode* m_Device          = nullptr;

    usize        m_BlockSize       = 0;
    usize        m_BlockCount      = 0;
    usize        m_BlockGroupCount = 0;
    usize        m_BlocksPerGroup  = 0;

    usize        LocateFreeGroup(usize blockCount = 1);
    usize        AllocateINodeInGroup(usize                       blockGroupIndex,
                                      Ext2FsBlockGroupDescriptor& blockGroup);
};
