/*
 * Created by v1tr10l7 on 19.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Limits.hpp>
#include <API/Posix/dirent.h>
#include <Prism/String/StringBuilder.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.DirectoryEntry;

import :VFS.INode;
import :VFS.MountPoint;
import :VFS.VFS;

// NOTE(v1tr10l7): Directories should only have one DirectoryEntry, regular
// files might have multiple pointing to the same inode

DirectoryEntry::DirectoryEntry(StringView name)
    : m_Name(name)
{
}
DirectoryEntry::DirectoryEntry(::WeakRef<DirectoryEntry> parent,
                               StringView                name)
    : m_Name(name)
    , m_Parent(parent)
{
}
DirectoryEntry::DirectoryEntry(const DirectoryEntry& other)
    : m_INode(other.m_INode)
{
}
DirectoryEntry::~DirectoryEntry() {}

::Ref<INode> DirectoryEntry::INode() const { return m_INode; }

Path         DirectoryEntry::Path()
{
    StringBuilder pathBuilder;

    WeakRef       current   = this;
    auto          rootEntry = VFS::RootDirectoryEntry();
    auto          rootINode = rootEntry->INode();

    while (current && current->INode() != rootINode)
    {
        auto segment = "/"_s;
        segment += current->m_Name;

        if (current->m_Name != "/"_sv) pathBuilder.Insert(segment);
        current = current->GetEffectiveParent().Raw();
    }

    return pathBuilder.Empty() ? "/"_s : pathBuilder.ToString();
}
const UnorderedMap<String, ::Ref<DirectoryEntry>>&
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

    if (!(m_Flags & DirectoryEntryFlags::eEntryTypeMask))
        m_Flags |= DirectoryEntryFlags::eDirectory;
    m_Flags |= DirectoryEntryFlags::eMountPoint;
}
void DirectoryEntry::Bind(::Ref<class INode> inode)
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
    Assert(entry);

    auto it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return;

    m_Children.Erase(it);
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
    auto                  rootEntry   = VFS::RootDirectoryEntry();
    ::Ref<DirectoryEntry> parentEntry = nullptr;

    MountPoint::Iterate(
        [this, &parentEntry](::Ref<MountPoint> mountPoint) -> IterationResult
        {
            if (mountPoint->GuestEntry() == this)
            {
                parentEntry = mountPoint->HostEntry();
                return IterationResult::eBreak;
            }

            return IterationResult::eContinue;
        });
    if (this == rootEntry.Raw() || this == VFS::RootDirectoryEntry().Raw())
        return const_cast<DirectoryEntry*>(this);
    else if (parentEntry) return parentEntry;

    return Parent();
}

ErrorOr<void>
DirectoryEntry::TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                    DirectoryIterator           iterator)
{
    PopulateDirectoryEntries();

    usize index = 0;
    for (auto& [name, entry] : m_Children)
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
    if (!INode()) return Error(ENOENT);

    Vector<StringView> inodes;
    DirectoryIterator  iterator;
    iterator.BindLambda(
        [&inodes](StringView name, loff_t offset, usize ino, u64 type) -> bool
        {
            inodes.PushBack(name);

            return true;
        });

    INode()->TraverseDirectories(nullptr, iterator);
    for (const auto name : inodes) Lookup(name);

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
