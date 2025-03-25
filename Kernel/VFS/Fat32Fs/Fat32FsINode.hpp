/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/INode.hpp>

class Fat32FsINode : public INode
{
  public:
    Fat32FsINode(INode* parent, std::string_view name, Filesystem* fs,
                 mode_t mode);

    virtual ~Fat32FsINode() {}

    virtual std::unordered_map<std::string_view, INode*>&
                  GetChildren() override;

    virtual void  InsertChild(INode* node, std::string_view name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

    virtual ErrorOr<void>  ChMod(mode_t mode) override { return Error(ENOSYS); }

    friend class Fat32Fs;

  private:
    usize              m_Cluster         = 0;
    usize              m_DirectoryOffset = 0;
    std::atomic<usize> m_NextIndex       = 2;
};
