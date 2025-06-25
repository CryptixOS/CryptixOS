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
    Fat32FsINode(INode* parent, StringView name, Filesystem* fs, mode_t mode);

    virtual ~Fat32FsINode() {}

    virtual const std::unordered_map<StringView, INode*>& Children() const;

    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

    virtual ErrorOr<void>  ChMod(mode_t mode) override { return Error(ENOSYS); }

    friend class Fat32Fs;

  private:
    class Fat32Fs*                         m_Fat32Fs         = nullptr;
    usize                                  m_Cluster         = 0;
    usize                                  m_DirectoryOffset = 0;
    Atomic<usize>                          m_NextIndex       = 2;

    std::unordered_map<StringView, INode*> m_Children;
};
