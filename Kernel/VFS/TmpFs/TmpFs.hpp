/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/INode.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>
#include <VFS/VFS.hpp>

class TmpFs : public Filesystem
{
  public:
    TmpFs(u32 flags);

    inline usize   GetSize() const { return m_Size; }
    inline usize   GetMaxSize() const { return m_MaxSize; }

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name, void* data = nullptr) override;
    virtual INode* CreateNode(INode* parent, std::string_view name,
                              mode_t mode) override;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override;
    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override;
    virtual bool   Populate(INode* node) override { return false; }

  private:
    usize m_MaxInodeCount = 0;
    usize m_MaxSize       = 0;

    usize m_Size          = 0;
};
