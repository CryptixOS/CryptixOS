/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>
#include <VFS/Filesystem.hpp>

#include <unordered_map>

class DevTmpFs : public Filesystem
{
  public:
    DevTmpFs(u32 flags);

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

    virtual ErrorOr<INode*> MkNod(INode* parent, DirectoryEntry* entry,
                                  mode_t mode, dev_t dev) override;

    static bool             RegisterDevice(Device* device);

  private:
    static std::unordered_map<dev_t, Device*> s_Devices;
};
