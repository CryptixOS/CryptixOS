/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Logger.hpp>
#include <VFS/INode.hpp>

class TmpFsINode final : public INode
{
  public:
    TmpFsINode(std::string_view name)
        : INode(name)
    {
        capacity = 128;
        data     = new u8[capacity];
    }
    TmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
               mode_t mode);

    ~TmpFsINode()
    {
        if (capacity > 0) delete data;
    }

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual void InsertChild(INode* node, std::string_view name) override
    {
        ScopedLock guard(m_Lock);
        m_Children[name] = node;
    }
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;

  private:
    u8*   data     = nullptr;
    usize capacity = 0;
};
