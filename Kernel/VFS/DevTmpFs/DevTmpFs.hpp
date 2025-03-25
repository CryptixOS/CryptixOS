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

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name,
                         const void*      data = nullptr) override;

    virtual INode* CreateNode(INode* parent, std::string_view name,
                              mode_t mode) override;

    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override;
    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override;
    virtual bool   Populate(INode* node) override { return true; }

    virtual INode* MkNod(INode* parent, std::string_view path, mode_t mode,
                         dev_t dev) override;

    static void    RegisterDevice(Device* device);

  private:
    static std::unordered_map<dev_t, Device*> s_Devices;
};
