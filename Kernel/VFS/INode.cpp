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
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>

INode::INode(StringView name)
    : m_Name(name)
{
}
INode::INode(StringView name, class Filesystem* fs)
    : m_Name(name)
    , m_Filesystem(fs)
{
    Thread*  thread  = CPU::GetCurrentThread();
    Process* process = thread ? thread->GetParent() : nullptr;

    // if (parent && parent->m_Stats.st_mode & S_ISUID)
    // {
    //     m_Stats.st_uid = parent->m_Stats.st_uid;
    //     m_Stats.st_gid = parent->m_Stats.st_gid;
    //
    //     return;
    // }

    if (!process) return;

    m_Stats.st_uid = process->m_Credentials.euid;
    m_Stats.st_gid = process->m_Credentials.egid;
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

    entry->m_INode         = node;
    node->m_DirectoryEntry = entry;

    return entry;
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
