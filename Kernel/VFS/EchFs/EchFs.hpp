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
    explicit EchFs(u32 flags)
        : Filesystem("EchFs", flags)
    {
    }
    virtual ~EchFs();

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;
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
