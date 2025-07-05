/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/Filesystem.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <VFS/TmpFs/TmpFsINode.hpp>

class TmpFs : public Filesystem
{
  public:
    TmpFs(u32 flags);

    inline usize GetSize() const { return m_Size; }
    inline usize GetMaxSize() const { return m_MaxSize; }

    virtual ErrorOr<Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<INode*> AllocateNode(StringView name,
                                         INodeMode  mode) override;
    virtual ErrorOr<INode*> CreateNode(INode* parent, Ref<DirectoryEntry> entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, Ref<DirectoryEntry> entry,
                                    StringView target) override;
    virtual INode*          Link(INode* parent, StringView name,
                                 INode* oldNode) override;
    virtual bool Populate(DirectoryEntry* dentry) override { return true; }
    virtual ErrorOr<INode*> MkNod(INode* parent, Ref<DirectoryEntry> entry,
                                  mode_t mode, dev_t dev) override;

  private:
    usize                  m_MaxInodeCount      = 0;
    usize                  m_MaxSize            = 0;

    usize                  m_Size               = 0;
    // FIXME(v1tr10l7): hardcoded for now
    usize                  m_FreeINodeCount     = PMM::PAGE_SIZE << 2;

    constexpr static usize DIRECTORY_ENTRY_SIZE = 20;
    friend class TmpFsINode;
};
