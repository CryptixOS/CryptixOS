/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
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
#include <Prism/String/StringBuilder.hpp>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
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

module Cryptix:Impl.VFS.INode;

import :Arch.CPU;
import :Scheduler.Process;
import :Scheduler.Thread;
import :VFS.FileDescriptor;
import :VFS.Filesystem;
import :VFS.INode;
import :VFS.MountPoint;

INode::INode(class Filesystem* fs)
    : m_Filesystem(fs)
{
}
INode::INode(StringView name)
    : m_Name(name)
{
}
INode::INode(StringView name, class Filesystem* fs)
    : m_Name(name)
    , m_Filesystem(fs)
{
    auto     thread  = CPU::GetCurrentThread();
    Process* process = thread ? thread->Parent() : nullptr;

    if (!process) return;

    m_Metadata.UID = process->Credentials().EffectiveUserID;
    m_Metadata.GID = process->Credentials().EffectiveGroupID;
}

const stat INode::Stats() const
{
    stat stats{};
    stats.st_dev     = m_Metadata.RootDeviceID;
    stats.st_ino     = m_Metadata.ID;
    stats.st_nlink   = m_Metadata.LinkCount;
    stats.st_mode    = m_Metadata.Mode;
    stats.st_uid     = m_Metadata.UID;
    stats.st_gid     = m_Metadata.GID;
    stats.st_rdev    = m_Metadata.DeviceID;
    stats.st_size    = m_Metadata.Size;
    stats.st_blksize = m_Metadata.BlockSize;
    stats.st_blocks  = m_Metadata.BlockCount;
    stats.st_atim    = m_Metadata.AccessTime;
    stats.st_mtim    = m_Metadata.ModificationTime;
    stats.st_ctim    = m_Metadata.ChangeTime;

    return stats;
}

bool INode::IsFilesystemRoot() const
{
    auto fsRootEntry = m_Filesystem->RootDirectoryEntry();
    auto fsRootINode = fsRootEntry->INode();

    return this == fsRootINode.Raw();
}

bool INode::IsEmpty()
{
    bool              hasEntries = false;
    DirectoryIterator iterator;
    iterator.BindLambda(
        [&](StringView name, loff_t offset, usize ino, usize type) -> bool
        {
            if (name != "."_sv && name != ".."_sv) hasEntries = true;
            return !hasEntries;
        });

    auto result = TraverseDirectories(nullptr, iterator);
    (void)result;
    return hasEntries;
}
bool INode::ReadOnly() { return false; }
bool INode::Immutable() { return false; }
bool INode::CanRead(const Credentials& creds) const
{
    if (creds.EffectiveUserID == 0 || m_Metadata.Mode & S_IROTH) return true;
    if (creds.EffectiveUserID == m_Metadata.UID && m_Metadata.Mode & S_IRUSR)
        return true;

    return m_Metadata.GID == creds.EffectiveGroupID
        && m_Metadata.Mode & S_IRGRP;
}
bool INode::CanWrite(const Credentials& creds) const
{
    if (creds.EffectiveUserID == 0 || m_Metadata.Mode & S_IWOTH) return true;
    if (creds.EffectiveUserID == m_Metadata.UID && m_Metadata.Mode & S_IWUSR)
        return true;

    return m_Metadata.GID == creds.EffectiveGroupID
        && m_Metadata.Mode & S_IWGRP;
}
bool INode::CanExecute(const Credentials& creds) const
{
    if (creds.EffectiveUserID == m_Metadata.UID && m_Metadata.Mode & S_IXUSR)
        return true;
    if (creds.EffectiveGroupID == m_Metadata.GID && m_Metadata.Mode & S_IXGRP)
        return true;
    if (m_Metadata.Mode & S_IXOTH) return true;
    return false;
}

bool INode::ValidatePermissions(const Credentials& creds, u32 acc)
{
    return true;
}

ErrorOr<File*> INode::Open(class ::Ref<::DirectoryEntry> dentry, i64 flags,
                           u64 accMode)
{
    return new File(this);
}
ErrorOr<::Ref<DirectoryEntry>> INode::CreateNode(::Ref<DirectoryEntry> entry,
                                                 INodeMode mode, dev_t dev)
{
    return Error(ENOSYS);
}
ErrorOr<::Ref<DirectoryEntry>> INode::CreateFile(::Ref<DirectoryEntry> entry,
                                                 INodeMode             mode)
{
    return Error(ENOSYS);
}
ErrorOr<::Ref<DirectoryEntry>>
INode::CreateDirectory(::Ref<DirectoryEntry> entry, INodeMode mode)
{
    return Error(ENOSYS);
}
ErrorOr<::Ref<DirectoryEntry>> INode::Symlink(::Ref<DirectoryEntry> entry,
                                              PathView              targetPath)
{
    return Error(ENOSYS);
}
ErrorOr<::Ref<DirectoryEntry>> INode::Link(::Ref<DirectoryEntry> oldEntry,
                                           ::Ref<DirectoryEntry> newEntry)
{
    return Error(ENOSYS);
}

ErrorOr<Path> INode::ReadLink() { return Error(ENOSYS); }
ErrorOr<void> INode::Unlink(::Ref<DirectoryEntry> entry)
{
    return Error(ENOSYS);
}

ErrorOr<isize> INode::CheckPermissions(INodeMode mask)
{
    if (mask & W_OK)
    {
        if (ReadOnly() && (IsRegular() || IsDirectory() || IsSymlink()))
            return Error(EROFS);
        if (Immutable()) return Error(EACCES);
    }

    return 0;
}

ErrorOr<::Ref<DirectoryEntry>> INode::Lookup(::Ref<DirectoryEntry> dentry)
{
    return Error(ENOSYS);
}

DeviceID  INode::BackingDeviceID() const { return m_Metadata.RootDeviceID; }
INodeID   INode::ID() const { return m_Metadata.ID; }
INodeMode INode::Mode() const { return m_Metadata.Mode & ~S_IFMT; }
nlink_t   INode::LinkCount() const { return m_Metadata.LinkCount; }
UserID    INode::UserID() const { return m_Metadata.UID; }
GroupID   INode::GroupID() const { return m_Metadata.GID; }
DeviceID  INode::DeviceID() const { return m_Metadata.DeviceID; }
isize     INode::Size() const { return m_Metadata.Size; }
blksize_t INode::BlockSize() const { return m_Metadata.BlockSize; }
blkcnt_t  INode::BlockCount() const { return m_Metadata.BlockCount; }

timespec  INode::AccessTime() const { return m_Metadata.AccessTime; }
timespec INode::ModificationTime() const { return m_Metadata.ModificationTime; }
timespec INode::StatusChangeTime() const { return m_Metadata.ChangeTime; }

ErrorOr<void> INode::SetOwner(::UserID uid, ::GroupID gid)
{
    ScopedLock guard(m_Lock);
    if (uid == m_Metadata.UID && gid == m_Metadata.GID) return {};
    m_Metadata.UID = uid;
    m_Metadata.GID = gid;

    m_Dirty        = true;
    return {};
}
ErrorOr<void> INode::ChangeMode(INodeMode mode)
{
    m_Metadata.Mode = (m_Metadata.Mode & ~07777u) | (mode & 07777u);
    m_Dirty         = true;

    return {};
}
ErrorOr<void> INode::UpdateTimestamps(timespec atime, timespec mtime,
                                      timespec ctime)
{
    if (atime) m_Metadata.AccessTime = atime;
    if (mtime) m_Metadata.ModificationTime = mtime;
    if (ctime) m_Metadata.ChangeTime = ctime;

    // TODO(v1tr10l7): Verify permissions
    m_Dirty = true;
    return {};
}
