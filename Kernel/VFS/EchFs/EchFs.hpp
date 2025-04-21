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

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name,
                         const void*      data = nullptr) override;
    virtual INode* CreateNode(INode* parent, std::string_view name,
                              mode_t mode) override;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override
    {
        return nullptr;
    }

    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override
    {
        return nullptr;
    }
    virtual bool Populate(INode* node) override;

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
