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
                       DirectoryEntry* entry, StringView name, const void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    if (m_Root) VFS::RecursiveDelete(m_Root);

    m_Root           = CreateNode(parent, entry, 0755 | S_IFDIR);
    auto targetEntry = target->DirectoryEntry();
    targetEntry->SetMountGate(m_Root, entry);

    m_MountedOn = target;
    return m_Root;
}

INode* DevTmpFs::CreateNode(INode* parent, DirectoryEntry* entry, mode_t mode,
                            uid_t uid, gid_t gid)
{
    auto inode = new DevTmpFsINode(parent, entry->Name(), this, mode);
    entry->Bind(inode);

    return inode;
}

INode* DevTmpFs::Symlink(INode* parent, DirectoryEntry* entry,
                         StringView target)
{
    ToDo();

    return nullptr;
}
INode* DevTmpFs::Link(INode* parent, StringView name, INode* oldNode)
{
    ToDo();

    return nullptr;
}

INode* DevTmpFs::MkNod(INode* parent, DirectoryEntry* entry, mode_t mode,
                       dev_t dev)
{
    auto it = s_Devices.find(dev);
    if (it == s_Devices.end()) return_err(nullptr, EEXIST);
    Device* device = it->second;

    auto    inode  = reinterpret_cast<DevTmpFsINode*>(
        CreateNode(parent, entry, mode, 0, 0));
    inode->m_Device = device;

    return inode;
}

bool DevTmpFs::RegisterDevice(Device* device)
{
    Assert(device);

    if (s_Devices.contains(device->GetID())) return false;

    s_Devices[device->GetID()] = device;
    return true;
}
