/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.SynthFsINode;

import :Scheduler.Process;
import :Time.Time;
import :VFS.Filesystem;
import :VFS.SynthFsINode;

SynthFsINode::SynthFsINode(StringView name, class Filesystem* fs, INodeID id,
                           INodeMode mode)
    : INode(name, fs)
{
    m_Metadata.ID           = id;
    m_Metadata.Mode         = mode;
    // Every directory has at least two entries: '.' and '..'
    m_Metadata.Size         = IsDirectory() ?: DIRECTORY_ENTRY_SIZE * 2;
    m_Metadata.LinkCount    = 1 + IsDirectory();

    m_Metadata.BlockSize    = PMM::PAGE_SIZE;
    m_Metadata.BlockCount   = 0;

    m_Metadata.RootDeviceID = fs->BackingDeviceID();
    m_Metadata.DeviceID     = 0;

    auto process            = Process::Current();
    if (process)
    {
        const auto& creds = process->Credentials();
        m_Metadata.GID    = creds.GroupID;
        m_Metadata.UID    = creds.UserID;
    }

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
}

ErrorOr<void>
SynthFsINode::TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                  DirectoryIterator           iterator)
{
    ScopedLock guard(m_Lock);
    usize      offset = 0;
    for (const auto& [name, inode] : Children())
    {
        usize     ino  = inode->ID();
        INodeMode mode = inode->Mode();
        auto      type = IF2DT(mode);

        if (!iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}
ErrorOr<::Ref<DirectoryEntry>> SynthFsINode::Lookup(::Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);

    auto       child = Children().Find(entry->Name());
    if (child != Children().end())
    {
        entry->Bind(child->Value);
        return entry;
    }

    return Error(ENOENT);
}

ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateNode(::Ref<DirectoryEntry> entry, INodeMode mode, dev_t dev)
{
    ScopedLock guard(m_Lock);
    if (m_Children.Contains(entry->Name())) return Error(EEXIST);
    auto inode = TryOrRet(m_Filesystem->AllocateNode(entry->Name(), mode))
                     .As<SynthFsINode>();

    inode->m_Parent = this;
    if (inode->IsRegular())
    {
        inode->m_Metadata.Size = inode->GetDefaultSize();
        inode->m_Buffer.Resize(GetDefaultSize());
    }

    inode->m_Metadata.DeviceID = dev;
    m_Metadata.Size += DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    m_Lock.Release();
    InsertChild(inode, entry->Name());
    m_Lock.Acquire();

    if (Mode() & S_ISGID)
    {
        inode->m_Metadata.GID = m_Metadata.GID;
        if (inode->IsDirectory()) inode->m_Metadata.Mode |= S_ISGID;
    }

    entry->Bind(inode);
    return entry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateFile(::Ref<DirectoryEntry> entry, INodeMode mode)
{
    return CreateNode(entry, (mode & ~S_IFMT) | S_IFREG, 0);
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::CreateDirectory(::Ref<DirectoryEntry> entry, INodeMode mode)
{

    TryOrRet(CreateNode(entry, (mode & ~S_IFMT) | S_IFDIR, 0));

    ++m_Metadata.LinkCount;
    return entry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::Symlink(::Ref<DirectoryEntry> entry, PathView targetPath)
{
    auto dentry     = TryOrRet(CreateNode(entry, 0777 | S_IFLNK, 0));
    auto inode      = dentry->INode().As<SynthFsINode>();

    inode->m_Target = targetPath;
    return dentry;
}
ErrorOr<::Ref<DirectoryEntry>>
SynthFsINode::Link(::Ref<DirectoryEntry> oldEntry, ::Ref<DirectoryEntry> entry)
{
    auto inode = oldEntry->INode().As<SynthFsINode>();
    if (inode->IsDirectory()) return Error(EPERM);

    m_Metadata.Size += DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    inode->m_Metadata.ChangeTime = currentTime;
    ++inode->m_Metadata.LinkCount;

    entry->Bind(inode);
    return entry;
}

void SynthFsINode::InsertChild(::Ref<INode> inode, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = inode;
}

isize SynthFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);
    bytes = Min(bytes, m_Buffer.Size() - offset);

    if (m_Filesystem->ShouldUpdateATime()) UpdateTimestamps(Time::GetReal());
    m_Buffer.Read(offset, reinterpret_cast<Byte*>(buffer), bytes);

    return bytes;
}
isize SynthFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);

    usize      newCapacity = m_Buffer.Size() ?: PMM::PAGE_SIZE;
    while (offset + bytes > newCapacity) newCapacity *= 2;

    ResizeBuffer(newCapacity);

    // TODO(v1tr10l7): Handle filesystem memory limits
    m_Buffer.Write(offset, reinterpret_cast<const Byte*>(buffer), bytes);
    auto currentTime = Time::GetReal();
    if (m_Filesystem->ShouldUpdateATime())
        UpdateTimestamps(currentTime, currentTime);

    return bytes;
}
ErrorOr<Path>  SynthFsINode::ReadLink() { return m_Target; }

ErrorOr<isize> SynthFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Buffer.Size()) return 0;

    RetOnError(ResizeBuffer(size));
    m_Buffer.ShrinkToFit();

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};
    UpdateTimestamps(atime, currentTime, currentTime);

    return 0;
}

ErrorOr<void> SynthFsINode::Rename(::Ref<INode> newParent, StringView newName)
{
    // TODO(v1tr10l7): Remove old inode

    // auto parent = reinterpret_cast<TmpFsINode*>(m_Parent);
    // parent->m_Children.Erase(m_Name);

    m_Name = newName;
    newParent->InsertChild(this, Name());

    return {};
}

ErrorOr<void> SynthFsINode::Unlink(::Ref<DirectoryEntry> entry)
{
    ScopedLock guard(m_Lock);
    auto       it = m_Children.Find(entry->Name());
    if (it == m_Children.end()) return {};

    auto inode = entry->INode().As<SynthFsINode>();
    m_Metadata.Size -= DIRECTORY_ENTRY_SIZE;

    auto currentTime = Time::GetReal();
    auto atime = m_Filesystem->ShouldUpdateATime() ? currentTime : timespec{};

    UpdateTimestamps(atime, currentTime, currentTime);
    inode->m_Metadata.ChangeTime = currentTime;
    --inode->m_Metadata.LinkCount;

    auto inodeLinkCount = inode->m_Metadata.LinkCount;
    if (inodeLinkCount == 0 + inode->IsDirectory())
    {
        m_Children.Erase(it);
        IgnoreUnused(m_Filesystem->FreeINode(inode));
    }

    auto parent = entry->Parent();
    Assert(parent);
    parent->RemoveChild(entry);

    return {};
}
ErrorOr<void> SynthFsINode::RmDir(::Ref<DirectoryEntry> entry)
{
    auto inode = entry->INode().As<SynthFsINode>();
    if (!inode->Children().Empty()) return Error(ENOTEMPTY);

    m_Lock.Acquire();
    --m_Metadata.LinkCount;
    m_Lock.Release();

    return Unlink(entry);
}

ErrorOr<void> SynthFsINode::ResizeBuffer(usize newSize)
{
    m_Buffer.Resize(newSize);
    m_Metadata.Size       = m_Buffer.Size();
    m_Metadata.BlockCount = Math::DivRoundUp(newSize, 512);

    return {};
}
