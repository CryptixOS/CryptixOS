/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Prism/String/StringBuilder.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

// FIXME(v1tr10l7): Reference counting, and cleaning up
// NOTE(v1tr10l7): Directories should only have one DirectoryEntry, regular
// files might have multiple pointing to the same inode

DirectoryEntry::DirectoryEntry(class INode* inode)
    : m_INode(inode)
{
    m_Name                    = inode->GetName();
    m_INode->m_DirectoryEntry = this;

    auto parent               = inode->GetParent();
    if (parent) m_Parent = parent->DirectoryEntry();
}
DirectoryEntry::DirectoryEntry(DirectoryEntry* parent, StringView name)
    : m_Name(name)
    , m_Parent(parent)
{
}

Path DirectoryEntry::Path() const
{
    StringBuilder pathBuilder;

    auto          current   = this;
    auto          rootEntry = VFS::GetRootDirectoryEntry();

    while (current && current != rootEntry)
    {
        auto segment = "/"_s;
        segment += current->m_Name;
        pathBuilder.Insert(segment);

        current = current->GetEffectiveParent();
    }

    return pathBuilder.Empty() ? "/"_s : pathBuilder.ToString();
}
const std::unordered_map<StringView, DirectoryEntry*>&
DirectoryEntry::Children() const
{
    return m_Children;
}

void DirectoryEntry::SetParent(DirectoryEntry* entry) { m_Parent = entry; }
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
        if (!next.Entry) return_err(nullptr, ENOENT);

        return next.Entry->FollowSymlinks();
    }

    return this;
}
DirectoryEntry* DirectoryEntry::GetEffectiveParent() const
{
    auto rootEntry = VFS::GetRootDirectoryEntry()->FollowMounts();

    if (this == rootEntry) return const_cast<DirectoryEntry*>(this);
    else if (this == m_INode->GetFilesystem()->RootDirectoryEntry())
        return m_INode->GetFilesystem()
            ->MountedOn()
            ->GetParent()
            ->DirectoryEntry();

    return m_INode->GetParent()->DirectoryEntry();
}
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
