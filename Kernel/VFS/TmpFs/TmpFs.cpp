/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Memory/PMM.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/TmpFs/TmpFs.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

TmpFs::TmpFs(u32 flags)
    : Filesystem("TmpFs", flags)
    , m_MaxInodeCount(0)
    , m_MaxSize(0)
    , m_Size(0)
{
}

ErrorOr<DirectoryEntry*> TmpFs::Mount(StringView sourcePath, const void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    m_MaxSize       = PMM::GetTotalMemory() / 2;
    m_MaxInodeCount = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;

    m_RootEntry     = new DirectoryEntry(nullptr, "/");
    auto inodeOr    = MkNod(nullptr, m_RootEntry, 0644 | S_IFDIR, 0);
    if (!inodeOr) goto fail;

    m_Root = inodeOr.value();
    m_RootEntry->Bind(m_Root);

    return m_RootEntry;
fail:
    delete m_RootEntry;
    return Error(inodeOr.error());
}
ErrorOr<INode*> TmpFs::CreateNode(INode* parent, DirectoryEntry* entry,
                                  mode_t mode, uid_t uid, gid_t gid)
{
    auto inodeOr = MkNod(parent, entry, mode, 0);
    if (!inodeOr) return Error(inodeOr.error());

    auto inode            = reinterpret_cast<TmpFsINode*>(inodeOr.value());
    inode->m_Stats.st_uid = uid;
    inode->m_Stats.st_gid = gid;

    return inode;
}

ErrorOr<INode*> TmpFs::Symlink(INode* parent, DirectoryEntry* entry,
                               StringView target)
{
    if (m_NextInodeIndex >= m_MaxInodeCount) return Error(ENOSPC);

    mode_t mode    = S_IFLNK | 0777;
    auto   inodeOr = CreateNode(parent, entry, mode, 0, 0);
    if (!inodeOr) return Error(inodeOr.error());

    auto inode      = reinterpret_cast<TmpFsINode*>(inodeOr.value());
    inode->m_Target = target;

    parent->InsertChild(parent, entry->Name());
    return inode;
}

INode* TmpFs::Link(INode* parent, StringView name, INode* oldNode)
{
    if (oldNode->IsDirectory())
    {
        errno = EISDIR;
        return nullptr;
    }

    return new TmpFsINode(name, this, (oldNode->Stats().st_mode & ~S_IFMT));
}

ErrorOr<INode*> TmpFs::MkNod(INode* parent, DirectoryEntry* entry, mode_t mode,
                             dev_t dev)
{
    if (m_NextInodeIndex >= m_MaxInodeCount
        || (S_ISREG(mode) && m_Size + TmpFsINode::GetDefaultSize() > m_MaxSize))
        return Error(ENOSPC);

    auto inode = new TmpFsINode(entry->Name(), this, mode, 0, 0);
    entry->Bind(inode);

    if (parent) parent->InsertChild(inode, entry->Name());
    return inode;
}
