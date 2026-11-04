/*
 * Created by v1tr10l7 on 16.01.2025.
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
#include <Prism/Core/Singleton.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.FileDescriptor;

import :Arch.CPU;
import :Scheduler.Process;
import :Scheduler.Scheduler;
import :Scheduler.Thread;
import :VFS.INode;
import :VFS.FileDescriptor;
import :VFS.MountPoint;
import :VFS.VFS;

void DirectoryEntries::Push(StringView name, loff_t offset, usize ino,
                            usize type)
{
    auto reclen
        = Math::AlignUp(DIRENT_LENGTH + name.Size() + 1, alignof(dirent));

    auto* entry     = reinterpret_cast<dirent*>(malloc(reclen));
    entry->d_ino    = ino;
    entry->d_off    = reclen;
    entry->d_reclen = reclen;
    entry->d_type   = type;

    name.Copy(entry->d_name, name.Size() + 1);
    reinterpret_cast<char*>(entry->d_name)[name.Size()] = 0;

    Entries.PushBack(entry);
    Size += entry->d_reclen;
}
usize DirectoryEntries::CopyAndPop(u8* out, usize capacity)
{
    if (Entries.Empty()) return 0;

    auto  entry     = Entries.Front();
    usize entrySize = entry->d_reclen;
    if (capacity < entrySize) return 0;

    Entries.PopFront();
    if (!entry || !(char*)entry->d_name) return 0;

    Memory::Copy(out, entry, entry->d_reclen);
    delete entry;

    return entrySize;
}

FileDescriptor::FileDescriptor(class ::Ref<::DirectoryEntry> dentry, i32 flags,
                               FileAccessMode accMode)
    : m_DirectoryEntry(dentry)
    , m_DirectoryIterator(dentry->begin())
{
    auto inode = dentry->INode();
    if (inode) m_File = new class File(inode);
    dentry->PopulateDirectoryEntries();
    m_DirectoryIterator = dentry->begin();

    m_DescriptionFlags  = flags
                       & ~(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY
                           | O_NOFOLLOW | O_TRUNC | O_CLOEXEC);
    m_AccessMode = accMode;
    m_Flags      = flags & O_CLOEXEC;
}
FileDescriptor::FileDescriptor(class ::Ref<::DirectoryEntry> dentry,
                               class File* file, i32 flags,
                               FileAccessMode accMode)
    : m_DirectoryEntry(dentry)
    , m_File(file)
    , m_DirectoryIterator(dentry->begin())
{
    m_DescriptionFlags = flags;
    // & ~(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY
    //     | O_NOFOLLOW | O_TRUNC | O_CLOEXEC);
    m_AccessMode       = accMode;
    m_Flags            = flags & O_CLOEXEC;
}

FileDescriptor::~FileDescriptor()
{
    bool writer = CanWrite();
    if (m_File && m_File->IsFifo()) m_File->Close(writer);
}

ErrorOr<isize> FileDescriptor::Read(const UserBuffer& out, usize count,
                                    isize offset)
{
    ScopedLock guard(m_Lock);

    if (!CanRead()) return Error(EBADF);
    if (!m_File)
    {
        LogError("FileDescriptor::Read: m_File is nullptr");
        return Error(ENOENT);
    }
    if (m_File->IsDirectory()) return Error(EISDIR);

    // if (WouldBlock())
    // {
    //     if (IsNonBlocking()) return Error(IsSocket() ? EWOULDBLOCK : EAGAIN);
    //     Thread* current = Thread::Current();
    //     Scheduler::Block(current);
    //
    //     if (current->WasInterrupted()) return Error(EINTR);
    // }

    if (offset < 0) offset = m_Offset;
    isize bytesRead = m_File->Read(out.Raw(), offset, count).ValueOr(0);
    offset += bytesRead;

    m_Offset = offset;
    return bytesRead;
}
ErrorOr<isize> FileDescriptor::Write(const UserBuffer& in, usize count,
                                     isize offset)
{
    ScopedLock guard(m_Lock);

    if (!CanWrite()) return Error(EBADF);

    if (!m_File) return Error(ENOENT);
    if (offset < 0) offset = m_Offset;
    if (m_Flags & O_APPEND) TryOrRet(Seek(SEEK_END, 0));

    isize bytesWritten = m_File->Write(in.Raw(), offset, count).ValueOr(0);
    offset += bytesWritten;

    m_Offset = offset;
    return bytesWritten;
}
ErrorOr<isize> FileDescriptor::IoCtl(usize request, usize arg)
{
    if (!m_File) return Error(ENOENT);
    return m_File->IoCtl(request, arg);
}
ErrorOr<const stat> FileDescriptor::Stat() const
{
    if (!m_File) return Error(ENOENT);

    return m_File->Stat();
}

ErrorOr<isize> FileDescriptor::Seek(i32 whence, off_t offset)
{
    ScopedLock guard(m_Lock);
    auto       file = m_File;
    if (IsPipe() || file->IsSocket() || file->IsFifo()) return Error(ESPIPE);

    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0) return Error(EINVAL);
            m_Offset = offset;
            break;
        case SEEK_CUR:
        {
            isize newOffset = m_Offset + offset;
            if (newOffset >= NumericLimits<off_t>::Max())
                return Error(EOVERFLOW);
            if (newOffset < 0) return Error(EINVAL);
            m_Offset += offset;
            break;
        }
        case SEEK_END:
        {
            usize size = file->Size();
            if (static_cast<usize>(m_Offset) + size
                > NumericLimits<off_t>::Max())
                return Error(EOVERFLOW);

            usize fileSize = m_File->Size();
            m_Offset       = fileSize + offset;
            break;
        }

        default: return Error(EINVAL);
    };

    return m_Offset;
}

ErrorOr<isize> FileDescriptor::Truncate(off_t size)
{
    if (m_File->IsDirectory()) return Error(EISDIR);
    if (!(m_AccessMode & FileAccessMode::eWrite) || !m_File->IsRegular())
        return Error(EBADF);

    return m_File->Truncate(size);
}

bool FileDescriptor::IsCharDevice()
{
    return INode() && INode()->IsCharDevice();
}
bool FileDescriptor::IsFifo() { return INode() && INode()->IsFifo(); }
bool FileDescriptor::IsDirectory() { return DirectoryEntry()->IsDirectory(); }
bool FileDescriptor::IsRegular() { return DirectoryEntry()->IsRegular(); }
bool FileDescriptor::IsSymlink() { return DirectoryEntry()->IsSymlink(); }
bool FileDescriptor::IsSocket() { return File() && File()->IsSocket(); }

[[clang::no_sanitize("alignment")]] ErrorOr<isize>
FileDescriptor::GetDirEntries(dirent* const out, usize maxSize)
{
    ScopedLock guard(m_Lock);

    auto       entry = DirectoryEntry();
    if (!entry) return Error(ENOENT);
    if (!entry->IsDirectory()) return Error(ENOTDIR);

    DirectoryEntries& dirEntries = GetDirEntries();
    if (dirEntries.IsEmpty()) GenerateDirEntries();
    if (dirEntries.IsEmpty()) return 0;

    usize length = 0;
    for (const auto& entry : dirEntries) length += entry->d_reclen;
    length = Math::Min(maxSize, length) - sizeof(dirent);

    if (dirEntries.Front()->d_reclen > maxSize) return Error(EINVAL);

    bool end                    = dirEntries.ShouldRegenerate;
    dirEntries.ShouldRegenerate = false;

    usize bytes                 = 0;
    usize offset                = 0;

    while (offset < length)
    {
        bytes        = offset;

        usize copied = dirEntries.CopyAndPop(
            reinterpret_cast<u8*>(out) + offset, length - offset);

        if (copied == 0) break;
        offset += copied;
    }

    if (dirEntries.IsEmpty()) dirEntries.ShouldRegenerate = true;

    return end ? 0 : bytes;
}
bool FileDescriptor::GenerateDirEntries()
{
    DebugWarnIf(m_Lock.Test(),
                "FileDescriptor::GenerateDirEntries called without "
                "m_Description->Lock being acquired!");

    DirectoryEntries& dirEntries = GetDirEntries();
    dirEntries.Clear();

    auto current = DirectoryEntry();
    if (current->IsMountPoint())
    {
        auto mountPoint = MountPoint::Lookup(current);
        if (mountPoint) current = mountPoint->GuestEntry();
    }
    current = current->FollowSymlinks().Promote();

    if (m_DirectoryIterator == current->end())
    {
        m_DirectoryIterator = current->begin();
        m_Offset            = 0;
    }
    for (; m_DirectoryIterator != current->end(); m_Offset++)
    {
        auto entry = m_DirectoryIterator->Value;
        auto inode = entry->INode();
        if (!inode) continue;
        auto name = entry->Name();

        if (m_Offset == 0)
        {
            inode = current->INode();
            name  = "."_sv;
        }
        else if (m_Offset == 1)
        {
            auto parent = current->GetEffectiveParent();
            if (!parent) parent = current;

            inode = parent->INode();
            name  = ".."_sv;
        }
        else ++m_DirectoryIterator;

        auto   stats = inode->Stats();
        ino_t  ino   = stats.st_ino;
        mode_t mode  = stats.st_mode;
        auto   type  = IF2DT(mode);

        dirEntries.Push(name, m_Offset, ino, type);
    }

    return true;
}
