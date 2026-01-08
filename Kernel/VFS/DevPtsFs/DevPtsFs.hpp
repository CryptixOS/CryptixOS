/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/VFS.hpp>

class DevPtsFs : public Filesystem
{
  public:
    explicit DevPtsFs(u32 flags);
    virtual ~DevPtsFs();

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<::Ref<INode>> AllocateNode(StringView name,
                                               INodeMode  mode) override;
    virtual ErrorOr<void>         FreeINode(::Ref<INode> inode) override;

    virtual bool Populate(DirectoryEntry* dentry) override { return true; }
    virtual ErrorOr<void> Stats(statfs& stats) override;

  private:
    friend class DevPtsFsINode;

    ErrorOr<::Ref<INode>> CreatePTMXNode();
};
