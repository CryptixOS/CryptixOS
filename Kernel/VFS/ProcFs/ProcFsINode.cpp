/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/dirent.h>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/String/StringBuilder.hpp>
#include <Prism/String/StringUtils.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Singleton.hpp>
#include <API/Posix/linux/capability.h>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
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
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
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

module Cryptix:Impl.VFS.ProcFs.ProcFsINode;

import :Library.Logger;
import :Memory.PMM;
import :Scheduler.Scheduler;
import :Time.Time;
import :VFS.Filesystem;
import :VFS.ProcFs.ProcFsINode;

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
    LogTrace("ProcFsINode::Read: Reading {} bytes at offset {}", bytes, offset);
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
