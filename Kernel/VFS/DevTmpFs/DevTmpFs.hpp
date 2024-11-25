/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "VFS/Filesystem.hpp"

struct Device
{
    virtual isize Read(void* buffer, off_t offset, usize count);
};

class DevTmpFs : public Filesystem
{
  public:
    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name, void* data = nullptr) override;

    virtual INode* CreateNode(INode* parent, std::string_view name, mode_t mode,
                              INodeType type) override;
    virtual INode* CreateDeviceFile(INode* parent, std::string_view name,
                                    dev_t dev, mode_t mode);

    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override;
    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override;
    virtual bool   Populate(INode* node) override { return false; }
};
