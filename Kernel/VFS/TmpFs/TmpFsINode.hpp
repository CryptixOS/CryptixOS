/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Logger.hpp"
#include "VFS/INode.hpp"

#include <unordered_map>

class TmpFsINode final : public INode
{
  public:
    TmpFsINode(std::string_view name, INodeType type)
        : INode(name, type)
    {
        capacity = 128;
        data     = new u8[capacity];
    }
    TmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode, INodeType type)
        : INode(parent, name, fs, type)
    {
        if (S_ISREG(mode))
        {
            capacity = GetDefaultSize();
            data     = new u8[capacity];
        }
        stats.st_size    = 0;
        stats.st_blocks  = 0;
        stats.st_blksize = 512;
        stats.st_dev     = fs->GetDeviceID();
        stats.st_ino     = fs->GetNextINodeIndex();
        stats.st_mode    = mode;
        stats.st_nlink   = 1;

        // TODO(V1tri0l1744): Set st_atim, st_mtim, st_ctim
        stats.st_atim    = {0, 0};
        stats.st_mtim    = {0, 0};
        stats.st_ctim    = {0, 0};
    }

    ~TmpFsINode()
    {
        if (capacity > 0) delete data;
    }

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual void InsertChild(INode* node, std::string_view name) override
    {
        std::unique_lock guard(lock);
        children[name] = node;
    }
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;

  private:
    u8*                    data     = nullptr;
    [[maybe_unused]] usize size     = 0;
    usize                  capacity = 0;
};
