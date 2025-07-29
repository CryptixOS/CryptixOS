/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/PMM.hpp>
#include <System/Limits.hpp>
#include <Time/Time.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

using namespace System;

TmpFs::TmpFs(u32 flags)
    : Filesystem("TmpFs", flags)
    , m_MaxINodeCount(0)
    , m_MaxSize(0)
    , m_Size(0)
{
    m_ID = NextFilesystemID();
}

ErrorOr<::Ref<DirectoryEntry>> TmpFs::Mount(StringView  sourcePath,
                                            const void* data)
{
    m_BlockSize      = PMM::PAGE_SIZE;
    m_BytesLimit     = PMM::GetTotalMemory() / 2;

    m_MaxBlockCount  = 64_kib;
    m_UsedBlockCount = 0;

    m_MaxINodeCount  = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;
    m_FreeINodeCount = m_MaxINodeCount;

    m_MaxSize        = PMM::GetTotalMemory() / 2;

    m_RootEntry      = new DirectoryEntry(nullptr, "/");
    m_Root = TryOrRet(CreateNode(nullptr, m_RootEntry, 0644 | S_IFDIR, 0));

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    return m_RootEntry;
}

ErrorOr<INode*> TmpFs::AllocateNode(StringView name, mode_t mode)
{
    if (m_NextINodeIndex >= m_MaxINodeCount) return Error(ENOSPC);
    else if (FreeINodeCount() == 0) return Error(ENOSPC);
    --m_FreeINodeCount;

    auto inode = new TmpFsINode(name, this, mode);
    if (!inode) return Error(ENOMEM);
    // TODO(v1tr10l7): uid, gid

    inode->m_Metadata.ID               = NextINodeIndex();
    inode->m_Metadata.BlockSize        = PMM::PAGE_SIZE;
    inode->m_Metadata.BlockCount       = 0;
    inode->m_Metadata.RootDeviceID     = 0;

    auto currentTime                   = Time::GetReal();
    inode->m_Metadata.AccessTime       = currentTime;
    inode->m_Metadata.ModificationTime = currentTime;
    inode->m_Metadata.ChangeTime       = currentTime;

    if (S_ISDIR(mode))
    {
        ++inode->m_Metadata.LinkCount;
        inode->m_Metadata.Size = 2 * DIRECTORY_ENTRY_SIZE;
    }

    --m_FreeINodeCount;
    return inode;
}
ErrorOr<INode*> TmpFs::CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                                  mode_t mode, dev_t dev)
{
    if (parent)
    {
        auto maybeEntry = parent->CreateNode(entry, mode, 0);
        RetOnError(maybeEntry);

        return maybeEntry.Value()->INode();
    }

    return new TmpFsINode(entry->Name(), this, mode, 0, 0);
}

ErrorOr<void> TmpFs::Stats(statfs& stats)
{
    Memory::Fill(&stats, 0, sizeof(statfs));

    stats.f_type   = TMPFS_MAGIC;
    stats.f_bsize  = PMM::PAGE_SIZE;
    stats.f_blocks = m_MaxBlockCount;
    stats.f_bfree = stats.f_bavail = m_MaxBlockCount - m_UsedBlockCount;

    stats.f_files                  = m_MaxINodeCount;
    stats.f_ffree                  = m_FreeINodeCount / INODE_SIZE;
    stats.f_fsid                   = m_ID;
    stats.f_namelen                = Limits::FILE_NAME;
    stats.f_frsize                 = 0;
    stats.f_flags                  = m_Flags;

    return {};
}
