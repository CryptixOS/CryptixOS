/*
 * Created by v1tr10l7 on 24.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Ext2Fs/Ext2FsStructures.hpp>
#include <VFS/INode.hpp>

class Ext2FsINode : public INode
{
  public:
    Ext2FsINode(INode* parent, StringView name, class Ext2Fs* fs, mode_t mode);

    virtual ~Ext2FsINode() {}

    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

    virtual ErrorOr<void>  ChMod(mode_t mode) override;

    friend class Ext2Fs;

  private:
    Ext2Fs*         m_Fs;
    Ext2FsINodeMeta m_Meta;
};
