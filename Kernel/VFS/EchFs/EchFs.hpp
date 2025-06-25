/*
 * Created by v1tr10l7 on 30.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/EchFs/EchFsStructures.hpp>
#include <VFS/Filesystem.hpp>

class EchFsINode;
class EchFs final : public Filesystem
{
  public:
    EchFs(u32 flags)
        : Filesystem("EchFs", flags)
    {
    }
    virtual ~EchFs();

    virtual ErrorOr<DirectoryEntry*> Mount(StringView  sourcePath,
                                           const void* data = nullptr) override;
    virtual ErrorOr<INode*> CreateNode(INode* parent, DirectoryEntry* entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, DirectoryEntry* entry,
                                    StringView target) override
    {
        return nullptr;
    }

    virtual INode* Link(INode* parent, StringView name, INode* oldNode) override
    {
        return nullptr;
    }
    bool         Populate(EchFsINode* inode);
    virtual bool Populate(DirectoryEntry* dentry) override;

    isize ReadDirectoryEntry(EchFsDirectoryEntry& entry, u8* dest, isize offset,
                             usize bytes);

  private:
    EchFsIdentityTable* m_IdentityTable         = nullptr;
    EchFsINode*         m_NativeRoot            = nullptr;

    usize               m_AllocationTableOffset = 0;
    usize               m_AllocationTableSize   = 0;

    usize               m_MainDirectoryStart    = 0;
    usize               m_MainDirectoryEnd      = 0;
};
