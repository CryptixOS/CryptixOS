/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
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

ErrorOr<::Ref<DirectoryEntry>> DevPtsFs::Mount(StringView  sourcePath,
                                               const void* data)
{
    m_BlockSize                 = 1024;
    m_BytesLimit                = PMM::GetTotalMemory() / 2;

    m_RootEntry                 = CreateRef<DirectoryEntry>(nullptr, "/");
    m_Root                      = TryOrRet(AllocateNode("/", 0644 | S_IFDIR));

    auto inode                  = m_Root.As<DevPtsFsINode>();
    inode->m_Metadata.ID        = 1;
    inode->m_Metadata.LinkCount = 2;
    inode->m_Metadata.Mode      = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
    // TODO(v1tr10l7): Set magic

    auto ptmx                   = new PTYMultiplexer(PTMX_MINOR);

    auto ptmxEntry = CreateRef<DirectoryEntry>(m_RootEntry, "ptmx");
    auto ptmxINode
        = inode->CreateNode(ptmxEntry, DEVPTS_DEFAULT_PTMX_MODE, ptmx->ID());

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    return m_RootEntry;
}

ErrorOr<::Ref<INode>> DevPtsFs::AllocateNode(StringView name, INodeMode mode)
{
    // if (m_NextINodeIndex >= m_MaxINodeCount) return Error(ENOSPC);
    // else if (m_FreeINodeCount == 0) return Error(ENOSPC);

    auto inode = CreateRef<DevPtsFsINode>(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);

    // --m_FreeINodeCount;
    return inode;
}
ErrorOr<void> DevPtsFs::FreeINode(::Ref<INode> inode) { return Error(ENOSYS); }

ErrorOr<void> DevPtsFs::Stats(statfs& stats) { return Error(ENOSYS); }
