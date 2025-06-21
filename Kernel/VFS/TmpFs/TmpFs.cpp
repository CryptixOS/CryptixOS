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

INode* TmpFs::Mount(INode* parent, INode* source, INode* target,
                    DirectoryEntry* entry, StringView name, const void* data)
{
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    m_MaxSize       = PMM::GetTotalMemory() / 2;
    m_MaxInodeCount = PMM::GetTotalMemory() / PMM::PAGE_SIZE / 2;

    m_Root          = CreateNode(parent, entry, 0644 | S_IFDIR);

    if (m_Root) m_MountedOn = target;
    return m_Root;
}
INode* TmpFs::CreateNode(INode* parent, DirectoryEntry* entry, mode_t mode,
                         uid_t uid, gid_t gid)
{
    if (m_NextInodeIndex >= m_MaxInodeCount
        || (S_ISREG(mode) && m_Size + TmpFsINode::GetDefaultSize() > m_MaxSize))
    {
        errno = ENOSPC;

        return nullptr;
    }

    auto inode = new TmpFsINode(parent, entry->Name(), this, mode, uid, gid);
    entry->Bind(inode);

    return inode;
}

INode* TmpFs::Symlink(INode* parent, DirectoryEntry* entry, StringView target)
{
    if (m_NextInodeIndex >= m_MaxInodeCount) return_err(nullptr, ENOSPC);

    mode_t mode = S_IFLNK | 0777;
    auto   node
        = reinterpret_cast<TmpFsINode*>(CreateNode(parent, entry, mode, 0, 0));

    node->m_Target = target;
    return node;
}

INode* TmpFs::Link(INode* parent, StringView name, INode* oldNode)
{
    if (oldNode->IsDirectory())
    {
        errno = EISDIR;
        return nullptr;
    }

    return new TmpFsINode(parent, name, this,
                          (oldNode->GetStats().st_mode & ~S_IFMT));
}
