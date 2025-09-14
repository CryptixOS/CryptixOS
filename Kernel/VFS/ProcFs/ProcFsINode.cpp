/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>
#include <Memory/PMM.hpp>
#include <Prism/String/StringBuilder.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Scheduler/Scheduler.hpp>

#include <Time/Time.hpp>
#include <VFS/Filesystem.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

isize ProcFsProperty::Read(u8* outBuffer, off_t offset, usize size)
{
    if (static_cast<usize>(offset) >= Buffer.Size()) return 0;

    usize bytesCopied = Buffer.Copy(reinterpret_cast<char*>(outBuffer) + offset,
                                    Min(size, Buffer.Size() - offset));

    if (offset + bytesCopied >= Buffer.Size()) GenerateRecord();
    return bytesCopied;
}

ProcFsINode::ProcFsINode(StringView name, class Filesystem* fs, INodeID id,
                         INodeMode mode, ProcFsProperty* property)
    : INode(name, fs)
    , m_Property(property)
{
    Assert(!S_ISDIR(mode) || !m_Property);

    m_Metadata.ID               = m_Filesystem->NextINodeIndex();
    m_Metadata.Mode             = mode;

    m_Metadata.Size             = IsDirectory() ?: 20 * 2;
    m_Metadata.LinkCount        = 1 + IsDirectory();

    m_Metadata.BlockSize        = PMM::PAGE_SIZE;
    m_Metadata.BlockCount       = 0;

    m_Metadata.RootDeviceID     = m_Filesystem->BackingDeviceID();
    m_Metadata.DeviceID         = 0;

    m_Metadata.UID              = 0;
    m_Metadata.GID              = 0;

    m_Metadata.AccessTime       = Time::GetReal();
    m_Metadata.ModificationTime = Time::GetReal();
    m_Metadata.ChangeTime       = Time::GetReal();
}
ProcFsINode::ProcFsINode(StringView name, class Filesystem* fs, INodeMode mode,
                         ProcFsProperty* property)
    : ProcFsINode(name, fs, fs->NextINodeIndex(), mode, property)
{
}

ErrorOr<void>
ProcFsINode::TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                 DirectoryIterator           iterator)
{
    if (!m_Populated) m_Populated = Populate();

    usize offset = 0;
    for (const auto& [name, inode] : m_Children)
    {
        INodeID   ino  = inode->ID();
        INodeMode mode = inode->Mode();
        auto      type = IF2DT(mode);

        if (!iterator(name, offset, ino, type)) break;
        ++offset;
    }

    return {};
}
ErrorOr<::Ref<DirectoryEntry>> ProcFsINode::Lookup(::Ref<DirectoryEntry> entry)
{
    if (!m_Populated) m_Populated = Populate();
    ScopedLock guard(m_Lock);

    auto       child = m_Children.Find(entry->Name());
    if (child != m_Children.end())
    {
        entry->Bind(child->Value);
        return entry;
    }

    return Error(ENOENT);
}

void ProcFsINode::InsertChild(::Ref<INode> node, StringView name)
{
    ScopedLock guard(m_Lock);
    m_Children[name] = node;
}
isize ProcFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    u8* dest = reinterpret_cast<u8*>(buffer);

    return m_Property ? m_Property->Read(dest, offset, bytes) : -1;
}
isize ProcFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    return -1;
}
ErrorOr<isize> ProcFsINode::Truncate(usize size) { return Error(EROFS); }

ProcFsRootINode::ProcFsRootINode(StringView name, class Filesystem* fs)
    : ProcFsINode(name, fs, 2, 0755 | S_IFDIR)
{
}
bool ProcFsRootINode::Populate()
{
    Process::ForEach(
        [this](auto* process) -> IterationResult
        {
            auto      pid       = process->ID();
            String    pidString = StringUtils::ToString(pid);

            INodeID   id        = pid << 10;
            INodeMode mode      = S_IFDIR | 0755;

            if (!m_Children.Contains(pidString))
                InsertChild(new ProcFsProcessINode(pidString, m_Filesystem, id,
                                                   mode, pid),
                            pidString);

            return IterationResult::eContinue;
        });
    if (!m_Children.Contains("self"_sv))
    {
        auto selfLink = new ProcFsSelfLinkINode("self"_sv, m_Filesystem);
        InsertChild(selfLink, "self"_sv);
    }

    return true;
}
ProcFsSelfLinkINode::ProcFsSelfLinkINode(StringView name, class Filesystem* fs)
    : ProcFsINode(name, fs, fs->NextINodeIndex(), S_IFLNK | 0755)
{
}
ErrorOr<Path> ProcFsSelfLinkINode::ReadLink()
{
    auto          selfID = Process::Current()->ID();
    StringBuilder builder;
    builder << "/proc/"_s;
    builder << ToString(selfID);
    builder << "/"_s;

    return String(builder);
}

ProcFsProcessINode::ProcFsProcessINode(StringView name, class Filesystem* fs,
                                       INodeID id, INodeMode mode,
                                       ProcessID pid)
    : ProcFsINode(name, fs, id, mode)
    , m_ProcessID(pid)
{
    auto fdinfo = new ProcFsFdINode("fd", m_Filesystem, m_ProcessID);
    InsertChild(fdinfo, "fd");
}
bool ProcFsProcessINode::Populate() { return true; }

ProcFsFdINode::ProcFsFdINode(StringView name, class Filesystem* fs,
                             ProcessID pid)
    : ProcFsINode(name, fs, 2, 0755 | S_IFDIR)
    , m_ProcessID(pid)
{
}
bool ProcFsFdINode::Populate()
{
    auto process = Scheduler::GetProcess(m_ProcessID);
    if (!process) return false;
    for (auto [num, fd] : process->FdTable())
    {
        auto dentry = fd->DirectoryEntry();
        auto path   = dentry->Path();

        auto name   = StringUtils::ToString(num);
        if (m_Children.Contains(name)) continue;

        auto symlink
            = new ProcFsSymlinkINode(name, m_Filesystem, S_IFLNK | 0755, path);
        InsertChild(symlink, name);
    }

    return true;
}

ProcFsSymlinkINode::ProcFsSymlinkINode(StringView name, class Filesystem* fs,
                                       INodeMode mode, PathView target)
    : ProcFsINode(name, fs, mode)
    , m_Target(target)
{
}

ErrorOr<Path> ProcFsSymlinkINode::ReadLink() { return m_Target; }
