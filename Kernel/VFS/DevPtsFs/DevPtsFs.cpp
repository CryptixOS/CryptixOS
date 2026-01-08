/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/TTY/PTYMultiplexer.hpp>

#include <System/Limits.hpp>
#include <Time/Time.hpp>

#include <VFS/DevPtsFs/DevPtsFs.hpp>
#include <VFS/DevPtsFs/DevPtsFsINode.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

inline constexpr INodeMode DEVPTS_DEFAULT_MODE      = 0600;
constexpr usize            DEVPTS_DEFAULT_PTMX_MODE = 0000;
constexpr usize            PTMX_MINOR               = 2;

DevPtsFs::DevPtsFs(u32 flags)
    : Filesystem("DevPtsFs", flags)
{
}
DevPtsFs::~DevPtsFs() {}

ErrorOr<::Ref<DirectoryEntry>> DevPtsFs::Mount(StringView  sourcePath,
                                               const void* data)
{
    m_BlockSize          = 0x400;
    m_BytesLimit         = 0;

    m_RootEntry          = CreateRef<DirectoryEntry>(nullptr, "/");
    m_Root               = CreateRef<DevPtsFsINode>("/", this, 1, S_IFDIR);

    auto inode           = m_Root.As<DevPtsFsINode>();
    inode->m_Metadata.ID = 1;
    inode->m_Metadata.LinkCount = 2;
    inode->m_Metadata.Mode      = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;

    auto ptmxEntry              = TryOrRet(CreatePTMXNode());

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    return m_RootEntry;
}

ErrorOr<::Ref<INode>> DevPtsFs::AllocateNode(StringView name, INodeMode mode)
{
    return Error(ENOENT);
}
ErrorOr<void> DevPtsFs::FreeINode(::Ref<INode> inode) { return Error(ENOENT); }

ErrorOr<void> DevPtsFs::Stats(statfs& stats)
{
    using namespace System;
    Memory::Fill(&stats, 0, sizeof(statfs));

    stats.f_type   = DEVPTSFS_MAGIC;
    stats.f_bsize  = m_BlockSize;
    stats.f_blocks = 0;
    stats.f_bfree = stats.f_bavail = 0;

    stats.f_files                  = 0;
    stats.f_ffree                  = 0;
    stats.f_fsid                   = m_ID;
    stats.f_namelen                = Limits::FILE_NAME;
    stats.f_frsize                 = 0x400;
    stats.f_flags                  = m_Flags;

    LogError("STATFS");
    return {};
}

ErrorOr<::Ref<INode>> DevPtsFs::CreatePTMXNode()
{
    auto       root = m_Root.As<DevPtsFsINode>();
    ScopedLock guard(root->m_Lock);

    auto       dentry = CreateRef<DirectoryEntry>(m_RootEntry, "ptmx");
    root->m_Lock.Release();
    if (root->Lookup(dentry)) return Error(EEXIST);
    root->m_Lock.Acquire();

    auto      id    = NextINodeIndex();
    INodeMode mode  = DEVPTS_DEFAULT_PTMX_MODE;
    auto      inode = CreateRef<DevPtsFsINode>(dentry->Name(), this, id, mode);
    if (!inode) return Error(ENOMEM);

    auto ptmx = new PTYMultiplexer(PTMX_MINOR);
    DeviceManager::RegisterCharDevice(ptmx);

    inode->m_Parent            = m_Root.Raw();
    inode->m_Metadata.DeviceID = ptmx->ID();

    auto currentTime           = Time::GetReal();
    auto atime                 = ShouldUpdateATime() ? currentTime : timespec{};
    root->UpdateTimestamps(atime, currentTime, currentTime);

    root->m_Lock.Release();
    root->InsertChild(inode, dentry->Name());
    root->m_Lock.Acquire();

    if (root->Mode() & S_ISGID) inode->m_Metadata.GID = root->m_Metadata.GID;

    dentry->Bind(inode);
    return dentry;
}
