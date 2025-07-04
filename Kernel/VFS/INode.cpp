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

    // if (parent && parent->m_Stats.st_mode & S_ISUID)
    // {
    //     m_Stats.st_uid = parent->m_Stats.st_uid;
    //     m_Stats.st_gid = parent->m_Stats.st_gid;
    //
    //     return;
    // }

    if (!process) return;

    m_Stats.st_uid = process->Credentials().euid;
    m_Stats.st_gid = process->Credentials().egid;
}

mode_t INode::Mode() const { return m_Stats.st_mode & ~S_IFMT; }
bool   INode::IsFilesystemRoot() const
{
    auto fsRootEntry = m_Filesystem->RootDirectoryEntry();
    auto fsRootINode = fsRootEntry->INode();

    return this == fsRootINode;
}

bool INode::IsEmpty()
{
    // m_Filesystem->Populate(this);
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
bool INode::CanWrite(const Credentials& creds) const
{
    if (creds.euid == 0 || m_Stats.st_mode & S_IWOTH) return true;
    if (creds.euid == m_Stats.st_uid && m_Stats.st_mode & S_IWUSR) return true;

    return m_Stats.st_gid == creds.egid && m_Stats.st_mode & S_IWGRP;
}

bool INode::ValidatePermissions(const Credentials& creds, u32 acc)
{
    return true;
}

ErrorOr<Ref<DirectoryEntry>> INode::CreateNode(Ref<DirectoryEntry> entry,
                                               mode_t mode, dev_t dev)
{
    auto newEntry = m_Filesystem->CreateNode(this, entry.Raw(), mode);
    RetOnError(newEntry);

    return entry;
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
ErrorOr<Ref<DirectoryEntry>> INode::Link(Ref<DirectoryEntry> oldEntry,
                                         Ref<DirectoryEntry> newEntry)
{
    return Error(ENOSYS);
}

ErrorOr<void> INode::Unlink(Ref<DirectoryEntry> entry) { return Error(ENOSYS); }

ErrorOr<isize> INode::ReadLink(UserBuffer& outBuffer)
{
    if (!m_Target.Raw() || m_Target.Size() == 0) return Error(EINVAL);

    usize count = std::min(m_Target.Size(), outBuffer.Size());
    outBuffer.Write(m_Target.Raw(), count);

    return static_cast<isize>(count);
}

ErrorOr<DirectoryEntry*> INode::Lookup(class DirectoryEntry* entry)
{
    auto node = Lookup(entry->Name());
    if (!node) return Error(ENOENT);

    entry->m_INode = node;
    return entry;
}

ErrorOr<void> INode::SetOwner(uid_t uid, gid_t gid)
{
    if (uid == m_Attributes.UID && gid == m_Attributes.GID) return {};
    m_Attributes.UID = uid;
    m_Attributes.GID = gid;

    m_Dirty          = true;
    return {};
}
ErrorOr<void> INode::ChangeMode(mode_t mode)
{
    m_Attributes.Mode = mode;
    m_Dirty           = true;

    return {};
}
ErrorOr<void> INode::UpdateTimestamps(timespec atime, timespec mtime,
                                      timespec ctime)
{
    if (atime) m_Attributes.AccessTime = atime;
    if (mtime) m_Attributes.ModificationTime = mtime;
    if (ctime) m_Attributes.ChangeTime = ctime;

    // TODO(v1tr10l7): Verify permissions
    m_Dirty = true;
    return {};
}
