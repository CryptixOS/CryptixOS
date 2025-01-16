/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>
#include <Utility/NonCopyable.hpp>
#include <VFS/INode.hpp>

class DevTmpFsINode : public INode, NonCopyable<DevTmpFsINode>
{
  public:
    DevTmpFsINode(INode* parent, std::string_view name, Filesystem* fs,
                  mode_t mode, Device* device = nullptr);
    virtual ~DevTmpFsINode()
    {
        if (m_Capacity > 0) delete m_Data;
    }

    virtual void InsertChild(INode* node, std::string_view name) override
    {
        ScopedLock guard(m_Lock);
        m_Children[name] = node;
    }

    virtual ssize_t Read(void* buffer, off_t offset, size_t bytes) override;
    virtual ssize_t Write(const void* buffer, off_t offset,
                          size_t bytes) override;
    virtual i32     IoCtl(usize request, usize arg) override;

  private:
    Device* m_Device   = nullptr;
    u8*     m_Data     = nullptr;
    usize   m_Capacity = 0;
};
