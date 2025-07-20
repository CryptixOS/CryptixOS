/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <VFS/Filesystem.hpp>

class DevTmpFs : public Filesystem
{
  public:
    DevTmpFs(u32 flags);

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;

    virtual ErrorOr<INode*> AllocateNode(StringView name,
                                         INodeMode  mode) override;

    virtual bool Populate(DirectoryEntry* dentry) override { return true; }

    static bool             RegisterDevice(Device* device);

  private:
    static UnorderedMap<dev_t, Device*> s_Devices;
    friend class DevTmpFsINode;
};
