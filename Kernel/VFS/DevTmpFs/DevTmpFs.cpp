/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Time/Time.hpp>
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

UnorderedMap<dev_t, Device*> DevTmpFs::s_Devices{};

DevTmpFs::DevTmpFs(u32 flags)
    : Filesystem("DevTmpFs", flags)
{
}

ErrorOr<::Ref<DirectoryEntry>> DevTmpFs::Mount(StringView  sourcePath,
                                               const void* data)
{
    if (m_Root) VFS::RecursiveDelete(m_Root);

    m_RootEntry    = CreateRef<DirectoryEntry>(nullptr, "/");
    auto maybeRoot = AllocateNode(m_RootEntry->Name(), 0755 | S_IFDIR);
    RetOnError(maybeRoot);

    m_Root = maybeRoot.Value();
    m_RootEntry->Bind(m_Root);

    return m_RootEntry;
}

ErrorOr<INode*> DevTmpFs::AllocateNode(StringView name, mode_t mode)
{
    auto inode = new DevTmpFsINode(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);
    // TODO(v1tr10l7): uid, gid

    // --m_FreeINodeCount;
    return inode;
}

bool DevTmpFs::RegisterDevice(Device* device)
{
    Assert(device);

    if (s_Devices.Contains(device->ID())) return false;

    s_Devices[device->ID()] = device;
    return true;
}
Device* DevTmpFs::Lookup(dev_t id)
{
    auto device = s_Devices.Find(id);
    if (device != s_Devices.end()) return device->Value;

    return nullptr;
}
