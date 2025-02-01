/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/INode.hpp>

class EchFsINode : public INode
{
  public:
    EchFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode, usize offset)
        : INode(parent, name, fs)
        , m_Offset(offset)
    {
        (void)m_Offset;
    }

    virtual ~EchFsINode() {}

    virtual void  InsertChild(INode* node, std::string_view name) override {}
    virtual isize Read(void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Truncate(usize size) override { return -1; }

  private:
    usize m_Offset = 0;
};
