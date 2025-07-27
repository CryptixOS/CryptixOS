/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <API/Posix/dirent.h>
#include <Prism/String/StringBuilder.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/VFS.hpp>

// NOTE(v1tr10l7): Directories should only have one DirectoryEntry, regular
// files might have multiple pointing to the same inode

DirectoryEntry::DirectoryEntry(::WeakRef<DirectoryEntry> parent,
                               StringView                name)
    : m_Name(name)
    , m_Parent(parent)
{
}
DirectoryEntry::~DirectoryEntry() {}

Path DirectoryEntry::Path()
{
    StringBuilder pathBuilder;

    WeakRef       current   = this;
    auto          rootEntry = VFS::RootDirectoryEntry();

    while (current && current != rootEntry)
    {
        auto segment = "/"_s;
        segment += current->m_Name;

        if (current->m_Name != "/"_sv) pathBuilder.Insert(segment);

        current = current->GetEffectiveParent().Raw();
    }

    return pathBuilder.Empty() ? "/"_s : pathBuilder.ToString();
}
const UnorderedMap<StringView, ::Ref<DirectoryEntry>>&
DirectoryEntry::Children() const
{
    return m_Children;
}

void DirectoryEntry::SetParent(::WeakRef<DirectoryEntry> entry)
{
    m_Parent = entry;
}
void DirectoryEntry::SetMountGate(::Ref<DirectoryEntry> mountPoint)
{
    ScopedLock guard(m_Lock);
    m_MountGate = mountPoint;

    if (!(m_Flags & DirectoryEntryFlags::eEntryTypeMask))
        m_Flags |= DirectoryEntryFlags::eDirectory;
    m_Flags |= DirectoryEntryFlags::eMountPoint;
}
void DirectoryEntry::Bind(class INode* inode)
{
    Assert(inode);

    ScopedLock guard(m_Lock);
    m_INode = inode;

    if (inode->IsDirectory()) m_Flags |= DirectoryEntryFlags::eDirectory;
    if (inode->IsRegular()) m_Flags |= DirectoryEntryFlags::eRegular;
    if (inode->IsSymlink()) m_Flags |= DirectoryEntryFlags::eSymlink;
    if (inode->IsCharDevice() /*|| inode->IsBlockDevice()*/ || inode->IsSocket()
        || inode->IsFifo())
        m_Flags |= DirectoryEntryFlags::eSpecial;
}
void DirectoryEntry::InsertChild(::Ref<class DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);
    m_Children[entry->Name()] = entry;
}
void DirectoryEntry::RemoveChild(::Ref<class DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);

    auto       it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return;

    m_Children.Erase(it);
}

::WeakRef<DirectoryEntry> DirectoryEntry::FollowMounts()
{
    WeakRef current = this;
    while (current && current->m_MountGate) current = current->m_MountGate;

    return current;
}
::WeakRef<DirectoryEntry> DirectoryEntry::FollowSymlinks(usize cnt)
{
    auto target = TryOrRetVal(m_INode->ReadLink(), this);

    if (!target.Empty() && IsSymlink())
    {
        if (cnt >= SYMLOOP_MAX - 1) return_err(nullptr, ELOOP);

        auto next = VFS::ResolvePath(m_Parent.Promote(), target, true)
                        .ValueOr(VFS::PathResolution{})
                        .Entry;
        if (!next) return_err(nullptr, ENOENT);

        return next->FollowSymlinks(++cnt);
    }

    return this;
}
WeakRef<DirectoryEntry> DirectoryEntry::GetEffectiveParent()
{
    auto rootEntry  = VFS::RootDirectoryEntry()->FollowMounts();
    auto mountPoint = MountPoint::Lookup(const_cast<DirectoryEntry*>(this));

    if (this == rootEntry.Raw() || this == VFS::RootDirectoryEntry().Raw())
        return const_cast<DirectoryEntry*>(this);
    else if (mountPoint) return mountPoint->HostEntry()->Parent();

    return Parent();
}

ErrorOr<void>
DirectoryEntry::TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                    DirectoryIterator           iterator)
{
    PopulateDirectoryEntries();

    usize index = 0;
    for (const auto& [name, entry] : m_Children)
    {
        auto inode = entry->INode();
        if (!inode)
        {
            ++index;
            continue;
        }
        if (index < m_DirOffset)
        {
            ++index;
            continue;
        }

        auto stats = inode->Stats();
        auto mode  = stats.st_mode;
        auto type  = IF2DT(mode);
        if (!iterator(entry->Name(), m_DirOffset, stats.st_ino, type)) break;
        ++m_DirOffset;
        ++index;
    }

    if (m_DirOffset == m_Children.Size()) m_DirOffset = 0;

    return {};
}
::Ref<DirectoryEntry> DirectoryEntry::Lookup(const String& name)
{
    auto entryIt = m_Children.Find(name);
    if (entryIt != m_Children.end()) return entryIt->Value;
    if (!m_INode) return nullptr;

    ::Ref entry = CreateRef<DirectoryEntry>(nullptr, name);
    TryOrRetVal(m_INode->Lookup(entry), nullptr);

    entry->SetParent(this);
    InsertChild(entry);

    return entry;
}
ErrorOr<void> DirectoryEntry::PopulateDirectoryEntries()
{
    if (m_Populated) return {};

    DirectoryIterator iterator;
    iterator.BindLambda(
        [this](StringView name, loff_t offset, usize ino, u64 type) -> bool
        {
            if (!m_Children.Contains(name)) Lookup(name);

            return true;
        });

    m_Populated = true;
    return {};
}

bool DirectoryEntry::IsMountPoint() const
{
    return m_Flags & DirectoryEntryFlags::eMountPoint;
}
bool DirectoryEntry::IsDirectory() const
{
    return m_Flags & DirectoryEntryFlags::eDirectory;
}
bool DirectoryEntry::IsRegular() const
{
    return m_Flags & DirectoryEntryFlags::eRegular;
}
bool DirectoryEntry::IsSymlink() const
{
    return m_INode->IsSymlink();
    // return m_Flags & DirectoryEntryFlags::eSymlink;
}
bool DirectoryEntry::IsSpecial() const
{
    return m_Flags & DirectoryEntryFlags::eSpecial;
}
