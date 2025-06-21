/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

void DirectoryEntry::Bind(class INode* inode)
{
    m_INode                 = inode;
    inode->m_DirectoryEntry = this;
}

DirectoryEntry* DirectoryEntry::FollowMounts()
{
    auto current = this;
    while (current && current->m_MountGate) current = current->m_MountGate;

    return current;

    auto inode = m_INode;
    if (!inode) return nullptr;

    // while (inode->mountGate) inode = inode->mountGate;
    return inode->DirectoryEntry();
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
    if (!m_INode) return nullptr;
    auto inode = m_INode->Lookup(name);
    if (!inode) return nullptr;

    return inode->DirectoryEntry();

    auto entry = m_Children.find(name);
    if (entry != m_Children.end()) return entry.operator->()->second;

    auto newEntry = new DirectoryEntry(name);
    auto result   = m_INode->Lookup(newEntry);
    if (result)
    {
        m_Children[name] = newEntry;
        return newEntry;
    }

    delete newEntry;
    return nullptr;
}
