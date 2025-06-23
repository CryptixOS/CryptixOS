/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <VFS/INode.hpp>

class TmpFsINode final : public INode
{
  public:
    TmpFsINode(INode* parent, StringView name, Filesystem* fs, mode_t mode,
               uid_t uid = 0, gid_t gid = 0);

    virtual ~TmpFsINode()
    {
        if (m_Capacity > 0) delete m_Data;
    }

    inline static constexpr usize GetDefaultSize() { return 0x1000; }

    virtual void InsertChild(INode* node, StringView name) override
    {
        ScopedLock guard(m_Lock);
        m_Children[name] = node;
    }
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<void> Rename(INode* newParent, StringView newName) override;
    virtual ErrorOr<void> MkDir(StringView name, mode_t mode, uid_t uid = 0,
                                gid_t gid = 0) override;
    virtual ErrorOr<void> Link(PathView path) override;
    virtual ErrorOr<void> ChMod(mode_t mode) override;

  private:
    u8*   m_Data     = nullptr;
    usize m_Capacity = 0;

    friend class TmpFs;
};
