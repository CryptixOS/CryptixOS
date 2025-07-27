/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Utility/Math.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

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
    if (inode) m_File = new File(inode);
    dentry->PopulateDirectoryEntries();
    m_DirectoryIterator = dentry->begin();

    m_DescriptionFlags  = flags
                       & ~(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY
                           | O_NOFOLLOW | O_TRUNC | O_CLOEXEC);
    m_AccessMode = accMode;
    m_Flags      = flags & O_CLOEXEC;
}
FileDescriptor::FileDescriptor(class ::Ref<::DirectoryEntry> dentry, File* file,
                               i32 flags, FileAccessMode accMode)
    : m_DirectoryEntry(dentry)
    , m_File(file)
    , m_DirectoryIterator(dentry->begin())
{
    m_DescriptionFlags = flags
                       & ~(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY
                           | O_NOFOLLOW | O_TRUNC | O_CLOEXEC);
    m_AccessMode = accMode;
    m_Flags      = flags & O_CLOEXEC;
}

FileDescriptor::~FileDescriptor() { delete m_File; }

ErrorOr<isize> FileDescriptor::Read(const UserBuffer& out, usize count,
                                    isize offset)
{
    ScopedLock guard(m_Lock);

    if (!CanRead()) return Error(EBADF);
    if (!m_File) return Error(ENOENT);
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

    isize bytesRead = m_File->Read(out, count, offset).ValueOr(0);
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

    isize bytesWritten = m_File->Write(in, count, offset).ValueOr(0);
    offset += bytesWritten;

    m_Offset = offset;
    return bytesWritten;
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

bool FileDescriptor::IsCharDevice() const
{
    return INode() && INode()->IsCharDevice();
}
bool FileDescriptor::IsFifo() const { return INode() && INode()->IsFifo(); }
bool FileDescriptor::IsDirectory() const
{
    return DirectoryEntry()->IsDirectory();
}
bool FileDescriptor::IsRegular() const { return DirectoryEntry()->IsRegular(); }
bool FileDescriptor::IsSymlink() const { return DirectoryEntry()->IsSymlink(); }
bool FileDescriptor::IsSocket() const { return INode() && INode()->IsSocket(); }

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

    auto current = DirectoryEntry()->FollowMounts()->FollowSymlinks();

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
