/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <System/Limits.hpp>
#include <Time/Time.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/DevTmpFs/DevTmpFsINode.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

DevTmpFs::DevTmpFs(u32 flags)
    : Filesystem("DevTmpFs", flags)
{
}

ErrorOr<::Ref<DirectoryEntry>> DevTmpFs::Mount(StringView  sourcePath,
                                               const void* data)
{
    m_BlockSize          = PMM::PAGE_SIZE;
    m_BytesLimit         = PMM::GetTotalMemory() / 2;

    m_MaxBlockCount      = 64_kib;
    m_UsedBlockCount     = 0;

    m_MaxINodeCount      = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;
    m_FreeINodeCount     = m_MaxINodeCount;

    m_MaxSize            = PMM::GetTotalMemory() / 2;

    m_RootEntry          = CreateRef<DirectoryEntry>(nullptr, "/");
    m_Root               = TryOrRet(AllocateNode("/", 0644 | S_IFDIR));

    auto inode           = reinterpret_cast<DevTmpFsINode*>(m_Root);
    inode->m_Metadata.ID = 2;

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    return m_RootEntry;
}

ErrorOr<INode*> DevTmpFs::AllocateNode(StringView name, mode_t mode)
{
    if (m_NextINodeIndex >= m_MaxINodeCount) return Error(ENOSPC);
    else if (m_FreeINodeCount == 0) return Error(ENOSPC);

    auto inode = new DevTmpFsINode(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);

    --m_FreeINodeCount;
    return inode;
}
ErrorOr<void> DevTmpFs::FreeINode(INode* inode)
{
    if (!inode) return Error(EFAULT);

    delete inode;
    ++m_FreeINodeCount;

    return {};
}

ErrorOr<void> DevTmpFs::Stats(statfs& stats)
{
    using namespace System;
    Memory::Fill(&stats, 0, sizeof(statfs));

    stats.f_type   = TMPFS_MAGIC;
    stats.f_bsize  = PMM::PAGE_SIZE;
    stats.f_blocks = m_MaxBlockCount;
    stats.f_bfree = stats.f_bavail = m_MaxBlockCount - m_UsedBlockCount;

    stats.f_files                  = m_MaxINodeCount;
    stats.f_ffree                  = m_FreeINodeCount;
    stats.f_fsid                   = m_ID;
    stats.f_namelen                = Limits::FILE_NAME;
    stats.f_frsize                 = 0;
    stats.f_flags                  = m_Flags;

    return {};
}
