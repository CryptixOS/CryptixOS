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

    inline usize            GetSize() const { return m_Size; }
    inline usize            GetMaxSize() const { return m_MaxSize; }

    virtual ErrorOr<INode*> Mount(INode* parent, INode* source, INode* target,
                                  DirectoryEntry* entry, StringView name,
                                  const void* data = nullptr) override;
    virtual ErrorOr<INode*> CreateNode(INode* parent, DirectoryEntry* entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, DirectoryEntry* entry,
                                    StringView target) override;
    virtual INode*          Link(INode* parent, StringView name,
                                 INode* oldNode) override;
    virtual bool            Populate(INode* node) override { return true; }

  private:
    usize m_MaxInodeCount = 0;
    usize m_MaxSize       = 0;

    usize m_Size          = 0;
};
