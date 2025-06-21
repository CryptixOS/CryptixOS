/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

// FIXME(v1tr10l7): Reference counting, and cleaning up
// NOTE(v1tr10l7): Directories should only have one DirectoryEntry, regular
// files might have multiple pointing to the same inode

void DirectoryEntry::SetMountGate(class INode*    inode,
                                  DirectoryEntry* mountPoint)
{
    ScopedLock guard(m_Lock);
    // m_INode                 = inode;
    // inode->m_DirectoryEntry = this;
    m_MountGate = mountPoint;
}
void DirectoryEntry::Bind(class INode* inode)
{
    ScopedLock guard(m_Lock);
    m_INode                 = inode;
    inode->m_DirectoryEntry = this;
}
void DirectoryEntry::InsertChild(class DirectoryEntry* entry)
{
    ScopedLock guard(m_Lock);
    m_Children[entry->Name()] = entry;
}

DirectoryEntry* DirectoryEntry::FollowMounts()
{
    auto current = this;
    while (current && current->m_MountGate) current = current->m_MountGate;

    return current;
}
DirectoryEntry* DirectoryEntry::FollowSymlinks()
{
    String target = "";
    target.Reserve(Limits::MAX_PATH_LENGTH);
    auto bufferOr = UserBuffer::ForUserBuffer(target.Raw(), target.Capacity());
    if (bufferOr && m_INode->ReadLink(bufferOr.value()))
        ;

    while (!target.Empty())
    {
        auto next = VFS::ResolvePath(m_Parent, target.Raw());
        if (!next.Node) return_err(nullptr, ENOENT);

        return next.Node->FollowSymlinks();
    }

    return this;
}
DirectoryEntry* DirectoryEntry::GetEffectiveParent() { return m_Parent; }
DirectoryEntry* DirectoryEntry::Lookup(const String& name)
{
    auto entryIt = m_Children.find(name);
    if (entryIt != m_Children.end()) return entryIt->second;
    if (!m_INode) return nullptr;

    auto inode = m_INode->Lookup(name);
    if (!inode) return nullptr;

    auto entry = inode->DirectoryEntry();
    InsertChild(entry);
    return entry;
}
