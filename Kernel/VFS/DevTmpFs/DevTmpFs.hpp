/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <VFS/Filesystem.hpp>

class DevTmpFs : public Filesystem
{
  public:
    explicit DevTmpFs(u32 flags);

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<INode*> AllocateNode(StringView name,
                                         INodeMode  mode) override;
    virtual ErrorOr<void>   FreeINode(INode* inode) override;

    virtual bool Populate(DirectoryEntry* dentry) override { return true; }
    virtual ErrorOr<void> Stats(statfs& stats) override;

  private:
    usize         m_MaxBlockCount  = 0;
    Atomic<usize> m_UsedBlockCount = 0;
    usize         m_MaxINodeCount  = 0;
    Atomic<usize> m_FreeINodeCount = PMM::PAGE_SIZE << 2;
    usize         m_MaxSize        = 0;

    usize         m_Size           = 0;

    friend class DevTmpFsINode;
};
