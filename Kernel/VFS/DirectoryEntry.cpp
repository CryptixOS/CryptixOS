/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Prism/String/StringBuilder.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/VFS.hpp>

// NOTE(v1tr10l7): Directories should only have one DirectoryEntry, regular
// files might have multiple pointing to the same inode

DirectoryEntry::DirectoryEntry(::Ref<DirectoryEntry> parent, class INode* inode)
    : m_INode(inode)
{
    m_Name   = inode->Name();
    m_Parent = parent;

    if (parent) parent->InsertChild(this);
}
DirectoryEntry::DirectoryEntry(::Ref<DirectoryEntry> parent, StringView name)
    : m_Name(name)
    , m_Parent(parent)
{
    if (parent) parent->InsertChild(this);
}
DirectoryEntry::~DirectoryEntry() {}

Path DirectoryEntry::Path() 
{
    StringBuilder pathBuilder;

    auto          current   = this;
    auto          rootEntry = VFS::GetRootDirectoryEntry();

    while (current && current != rootEntry.Raw())
    {
        auto segment = "/"_s;
        segment += current->m_Name;
        pathBuilder.Insert(segment);

        current = current->GetEffectiveParent().Raw();
    }

    return pathBuilder.Empty() ? "/"_s : pathBuilder.ToString();
}
const std::unordered_map<StringView, Prism::Ref<DirectoryEntry>>&
DirectoryEntry::Children() const
{
    return m_Children;
}

void DirectoryEntry::SetParent(::Ref<DirectoryEntry> entry) { m_Parent = entry; }
void DirectoryEntry::SetMountGate(class INode*    inode,
                                  ::Ref<DirectoryEntry> mountPoint)
{
    ScopedLock guard(m_Lock);
    m_MountGate = mountPoint;
}
void DirectoryEntry::Bind(class INode* inode)
{
    ScopedLock guard(m_Lock);
    m_INode = inode;
}
void DirectoryEntry::InsertChild(Prism::Ref<class DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);
    m_Children[entry->Name()] = entry;
}
void DirectoryEntry::RemoveChild(Prism::Ref<class DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);

    auto       it = m_Children.find(entry->Name());
    if (it == m_Children.end()) return;

    m_Children.erase(it);
}

::Ref<DirectoryEntry> DirectoryEntry::FollowMounts()
{
    auto current = this;
    while (current && current->m_MountGate) current = current->m_MountGate.Raw();

    return current;
}
::Ref<DirectoryEntry> DirectoryEntry::FollowSymlinks(usize cnt)
{
    auto target = m_INode->GetTarget();

    if (!target.Empty())
    {
        if (cnt >= SYMLOOP_MAX - 1) return_err(nullptr, ELOOP);

        auto next = VFS::ResolvePath(m_Parent.Raw(), target.Raw(), true)
                        .value_or(VFS::PathResolution{})
                        .Entry;
        if (!next) return_err(nullptr, ENOENT);

        return next->FollowSymlinks(++cnt);
    }

    return this;
}
WeakRef<DirectoryEntry> DirectoryEntry::GetEffectiveParent() 
{
    auto rootEntry  = VFS::GetRootDirectoryEntry()->FollowMounts();
    auto mountPoint = MountPoint::Lookup(const_cast<DirectoryEntry*>(this));

    if (this == rootEntry.Raw() || this == VFS::GetRootDirectoryEntry().Raw())
        return const_cast<DirectoryEntry*>(this);
    else if (mountPoint) return mountPoint->HostEntry()->Parent();

    return Parent();
}
::Ref<DirectoryEntry> DirectoryEntry::Lookup(const String& name)
{
    auto entryIt = m_Children.find(name);
    if (entryIt != m_Children.end()) return entryIt->second.Raw();
    if (!m_INode) return nullptr;

    auto inode = m_INode->Lookup(name);
    if (!inode) return nullptr;

    ::Ref entry = new DirectoryEntry(this, name);
    entry->Bind(inode);

    InsertChild(entry);
    return entry.Raw();
}
