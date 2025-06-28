/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>

class TmpFsINode final : public INode
{
  public:
    TmpFsINode(StringView name, class Filesystem* fs, mode_t mode,
               uid_t uid = 0, gid_t gid = 0);

    virtual ~TmpFsINode()
    {
        if (m_Capacity > 0) delete m_Data;
    }

    virtual ErrorOr<void>
                   TraverseDirectories(class DirectoryEntry* parent,
                                       DirectoryIterator     iterator) override;
    virtual INode* Lookup(const String& name) override;

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual const std::unordered_map<StringView, INode*>& Children() const
    {
        return m_Children;
    }
    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<void> Rename(INode* newParent, StringView newName) override;
    virtual ErrorOr<Ref<DirectoryEntry>> MkDir(Ref<DirectoryEntry> entry,
                                               mode_t mode) override;
    virtual ErrorOr<void>                Link(PathView path) override;
    virtual ErrorOr<void>                ChMod(mode_t mode) override;

  private:
    u8*                                    m_Data     = nullptr;
    usize                                  m_Capacity = 0;
    std::unordered_map<StringView, INode*> m_Children;

    friend class TmpFs;
};
