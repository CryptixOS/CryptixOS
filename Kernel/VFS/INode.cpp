/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Prism/String/StringBuilder.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/Filesystem.hpp>
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>

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
    Thread*  thread  = CPU::GetCurrentThread();
    Process* process = thread ? thread->Parent() : nullptr;

    if (!process) return;

    m_Metadata.UID = process->Credentials().euid;
    m_Metadata.GID = process->Credentials().egid;
}

const stat INode::Stats()
{
    stat stats{};
    stats.st_dev     = m_Metadata.DeviceID;
    stats.st_ino     = m_Metadata.ID;
    stats.st_nlink   = m_Metadata.LinkCount;
    stats.st_mode    = m_Metadata.Mode;
    stats.st_uid     = m_Metadata.UID;
    stats.st_gid     = m_Metadata.GID;
    stats.st_rdev    = m_Metadata.RootDeviceID;
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

    return this == fsRootINode;
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
bool INode::CanWrite(const Credentials& creds) const
{
    if (creds.euid == 0 || m_Metadata.Mode & S_IWOTH) return true;
    if (creds.euid == m_Metadata.UID && m_Metadata.Mode & S_IWUSR) return true;

    return m_Metadata.GID == creds.egid && m_Metadata.Mode & S_IWGRP;
}

bool INode::ValidatePermissions(const Credentials& creds, u32 acc)
{
    return true;
}

ErrorOr<Ref<DirectoryEntry>> INode::CreateNode(Ref<DirectoryEntry> entry,
                                               mode_t mode, dev_t dev)
{
    return Error(ENOSYS);
}
ErrorOr<Ref<DirectoryEntry>> INode::CreateFile(Ref<DirectoryEntry> entry,
                                               mode_t              mode)
{
    return Error(ENOSYS);
}
ErrorOr<Ref<DirectoryEntry>> INode::CreateDirectory(Ref<DirectoryEntry> entry,
                                                    mode_t              mode)
{
    return Error(ENOSYS);
}
ErrorOr<Ref<DirectoryEntry>> INode::Symlink(Ref<DirectoryEntry> entry,
                                            PathView            targetPath)
{
    return Error(ENOSYS);
}
ErrorOr<Ref<DirectoryEntry>> INode::Link(Ref<DirectoryEntry> oldEntry,
                                         Ref<DirectoryEntry> newEntry)
{
    return Error(ENOSYS);
}

ErrorOr<Path> INode::ReadLink() { return Error(ENOSYS); }
ErrorOr<void> INode::Unlink(Ref<DirectoryEntry> entry) { return Error(ENOSYS); }

ErrorOr<isize> INode::CheckPermissions(mode_t mask)
{
    if (mask & W_OK)
    {
        if (ReadOnly() && (IsRegular() || IsDirectory() || IsSymlink()))
            return Error(EROFS);
        if (Immutable()) return Error(EACCES);
    }

    return 0;
}

ErrorOr<Ref<DirectoryEntry>> INode::Lookup(Ref<DirectoryEntry> dentry)
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

ErrorOr<void> INode::SetOwner(uid_t uid, gid_t gid)
{
    if (uid == m_Metadata.UID && gid == m_Metadata.GID) return {};
    m_Metadata.UID = uid;
    m_Metadata.GID = gid;

    m_Dirty        = true;
    return {};
}
ErrorOr<void> INode::ChangeMode(mode_t mode)
{
    m_Metadata.Mode |= (mode & 0777);
    m_Dirty = true;

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
