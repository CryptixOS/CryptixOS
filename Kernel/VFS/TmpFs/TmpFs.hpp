/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "VFS/INode.hpp"
#include "VFS/TmpFs/TmpFsINode.hpp"
#include "VFS/VFS.hpp"

struct TmpFs : public Filesystem
{
    usize maxInodes   = 0;
    usize maxSize     = 0;

    usize currentSize = 0;

    TmpFs();

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name, void* data = nullptr) override;
    virtual INode* CreateNode(INode* parent, std::string_view name, mode_t mode,
                              INodeType type) override;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override;
    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override;
    virtual bool   Populate(INode* node) override { return false; }
};
