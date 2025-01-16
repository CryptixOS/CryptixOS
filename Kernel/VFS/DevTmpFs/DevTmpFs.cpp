/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>

std::unordered_map<dev_t, Device*> DevTmpFs::s_Devices{};

DevTmpFs::DevTmpFs(u32 flags)
    : Filesystem("DevTmpFs", flags)
{
}

INode* DevTmpFs::Mount(INode* parent, INode* source, INode* target,
                       std::string_view name, void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    if (m_Root) delete m_Root;
    m_Root      = CreateNode(parent, name, 0755 | S_IFDIR);
    m_MountedOn = target;

    return m_Root;
}

INode* DevTmpFs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    return new DevTmpFsINode(parent, name, this, mode);
}

INode* DevTmpFs::Symlink(INode* parent, std::string_view name,
                         std::string_view target)
{
    ToDo();

    return nullptr;
}
INode* DevTmpFs::Link(INode* parent, std::string_view name, INode* oldNode)
{
    ToDo();

    return nullptr;
}

INode* DevTmpFs::MkNod(INode* parent, std::string_view name, mode_t mode,
                       dev_t dev)
{
    auto it = s_Devices.find(dev);
    if (it == s_Devices.end()) return_err(nullptr, EEXIST);
    Device* device = it->second;

    return new DevTmpFsINode(parent, name, this, mode, device);
}

void DevTmpFs::RegisterDevice(Device* device)
{
    Assert(device);

    s_Devices[device->GetID()] = device;
}
