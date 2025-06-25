/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>
#include <VFS/DirectoryEntry.hpp>

std::unordered_map<dev_t, Device*> DevTmpFs::s_Devices{};

DevTmpFs::DevTmpFs(u32 flags)
    : Filesystem("DevTmpFs", flags)
{
}

ErrorOr<DirectoryEntry*> DevTmpFs::Mount(StringView  sourcePath,
                                         const void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    if (m_Root) VFS::RecursiveDelete(m_Root);

    m_RootEntry    = new DirectoryEntry(nullptr, "/");
    auto maybeRoot = MkNod(nullptr, m_RootEntry, 0755 | S_IFDIR, 0);
    if (!maybeRoot)
    {
        delete m_RootEntry;
        return Error(maybeRoot.error());
    }

    m_Root = maybeRoot.value();
    m_RootEntry->Bind(m_Root);

    return m_RootEntry;
}

ErrorOr<INode*> DevTmpFs::CreateNode(INode* parent, DirectoryEntry* entry,
                                     mode_t mode, uid_t uid, gid_t gid)
{
    auto inodeOr = MkNod(parent, entry, mode, 0);
    auto inode   = inodeOr.value_or(nullptr);
    if (!inode) return Error(inodeOr.error());

    entry->Bind(inode);
    return inode;
}

ErrorOr<INode*> DevTmpFs::Symlink(INode* parent, DirectoryEntry* entry,
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

ErrorOr<INode*> DevTmpFs::MkNod(INode* parent, DirectoryEntry* entry,
                                mode_t mode, dev_t dev)
{
    auto inode = new DevTmpFsINode(parent, entry->Name(), this, mode);
    if (!inode) return Error(ENOMEM);
    entry->Bind(inode);

    if (parent) parent->InsertChild(inode, entry->Name());

    auto it = s_Devices.find(dev);
    if (it == s_Devices.end()) return inode;

    inode->m_Device = it->second;
    return inode;
}

bool DevTmpFs::RegisterDevice(Device* device)
{
    Assert(device);

    if (s_Devices.contains(device->GetID())) return false;

    s_Devices[device->GetID()] = device;
    return true;
}
