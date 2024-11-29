/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "DevTmpFs.hpp"

#include "VFS/DevTmpFs/DevTmpFsINode.hpp"

std::unordered_map<dev_t, Device*> DevTmpFs::devices{};

DevTmpFs::DevTmpFs()
    : Filesystem("DevTmpFs")
{
}

INode* DevTmpFs::Mount(INode* parent, INode* source, INode* target,
                       std::string_view name, void* data)
{
    mountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    if (root) delete root;
    root      = CreateNode(parent, name, 0644 | S_IFDIR, INodeType::eDirectory);
    mountedOn = root;

    LogTrace("DevTmpFs: Created root node");
    return root;
}

INode* DevTmpFs::CreateNode(INode* parent, std::string_view name, mode_t mode,
                            INodeType type)
{
    return new DevTmpFsINode(parent, name, this, type);
}

INode* DevTmpFs::Symlink(INode* parent, std::string_view name,
                         std::string_view target)
{
    return nullptr;
}
INode* DevTmpFs::Link(INode* parent, std::string_view name, INode* oldNode)
{
    return nullptr;
}

INode* DevTmpFs::MkNod(INode* parent, std::string_view name, mode_t mode,
                       dev_t dev)
{
    auto it = devices.find(dev);
    if (it == devices.end()) return nullptr;
    Device* device = it->second;

    LogTrace("Creating character device");
    return new DevTmpFsINode(parent, name, this, INodeType::eCharacterDevice,
                             device);
}

void DevTmpFs::RegisterDevice(Device* device)
{
    Assert(device);

    devices[device->GetID()] = device;
}
