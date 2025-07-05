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

    std::memcpy(out, entry, entry->d_reclen);
    delete entry;

    return entrySize;
}

FileDescriptor::FileDescriptor(class DirectoryEntry* node, i32 flags,
                               FileAccessMode accMode)
{
    m_Description = new FileDescription;
    m_Description->IncRefCount();
    m_Description->Entry = node;

    m_Description->Flags = flags
                         & ~(O_CREAT | O_DIRECTORY | O_EXCL | O_NOCTTY
                             | O_NOFOLLOW | O_TRUNC | O_CLOEXEC);
    m_Description->AccessMode = accMode;
    m_Flags                   = flags & O_CLOEXEC;
}
FileDescriptor::FileDescriptor(FileDescriptor* fd, i32 flags)
    : m_Description(fd->m_Description)
    , m_Flags(flags)
{
    m_Description->IncRefCount();
}
FileDescriptor::~FileDescriptor()
{
    m_Description->DecRefCount();
    if (m_Description->RefCount == 0) delete m_Description;
}

ErrorOr<isize> FileDescriptor::Read(const UserBuffer& out, usize count,
                                    isize offset)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanRead()) return Error(EBADF);
    if (!DirectoryEntry()) return Error(ENOENT);
    if (DirectoryEntry()->IsDirectory()) return Error(EISDIR);

    if (WouldBlock())
    {
        if (IsNonBlocking()) return Error(IsSocket() ? EWOULDBLOCK : EAGAIN);
        Thread* current = Thread::Current();
        Scheduler::Block(current);

        if (current->WasInterrupted()) return Error(EINTR);
    }

    if (offset < 0) offset = m_Description->Offset;

    isize bytesRead = INode()->Read(out.Raw(), offset, count);
    offset += bytesRead;

    m_Description->Offset = offset;
    return bytesRead;
}
ErrorOr<isize> FileDescriptor::Write(const UserBuffer& in, usize count,
                                     isize offset)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanWrite()) return Error(EBADF);

    class INode* node = INode();
    if (!node) return Error(ENOENT);
    if (offset < 0) offset = m_Description->Offset;

    isize bytesWritten = node->Write(in.Raw(), offset, count);
    offset += bytesWritten;

    m_Description->Offset = offset;

    return bytesWritten;
}
ErrorOr<const stat> FileDescriptor::Stat() const
{
    class INode* node = INode();
    if (!node) return std::unexpected(Error(ENOENT));

    return node->Stats();
}

ErrorOr<isize> FileDescriptor::Seek(i32 whence, off_t offset)
{
    ScopedLock guard(m_Description->Lock);
    auto       entry = DirectoryEntry();
    if (IsPipe() || entry->IsSocket() || entry->IsFifo()) return Error(ESPIPE);

    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0) return Error(EINVAL);
            m_Description->Offset = offset;
            break;
        case SEEK_CUR:
        {
            isize newOffset = m_Description->Offset + offset;
            if (newOffset >= std::numeric_limits<off_t>::max())
                return Error(EOVERFLOW);
            if (newOffset < 0) return Error(EINVAL);
            m_Description->Offset += offset;
            break;
        }
        case SEEK_END:
        {
            usize size = INode()->Stats().st_size;
            if (static_cast<usize>(m_Description->Offset) + size
                > std::numeric_limits<off_t>::max())
                return Error(EOVERFLOW);
            m_Description->Offset = INode()->Stats().st_size + offset;
            break;
        }

        default: return Error(EINVAL);
    };

    return m_Description->Offset;
}

ErrorOr<isize> FileDescriptor::Truncate(off_t size)
{
    if (DirectoryEntry()->IsDirectory()) return Error(EISDIR);
    if (!(m_Description->AccessMode & FileAccessMode::eWrite)
        || !DirectoryEntry()->IsRegular())
        return Error(EBADF);

    return INode()->Truncate(size);
}

[[clang::no_sanitize("alignment")]] ErrorOr<i32>
FileDescriptor::GetDirEntries(dirent* const out, usize maxSize)
{
    ScopedLock guard(m_Description->Lock);

    auto       entry = DirectoryEntry();
    if (!entry) return Error(ENOENT);
    if (!entry->IsDirectory()) return Error(ENOTDIR);

    DirectoryEntries& dirEntries = GetDirEntries();
    if (dirEntries.IsEmpty()) GenerateDirEntries();
    if (dirEntries.IsEmpty()) return 0;

    usize length = 0;
    for (const auto& entry : dirEntries) length += entry->d_reclen;
    length = std::min(maxSize, length) - sizeof(dirent);

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
    DebugWarnIf(m_Description->Lock.Test(),
                "FileDescriptor::GenerateDirEntries called without "
                "m_Description->Lock being acquired!");

    DirectoryEntries& dirEntries = GetDirEntries();
    dirEntries.Clear();

    auto current = DirectoryEntry();
    while (current->IsSymlink() || current->m_MountGate)
    {
        if (current->m_MountGate)
        {
            current = current->FollowMounts();
            continue;
        }

        auto parentEntry = current->Parent() ? current->Parent() : nullptr;
        auto target      = current->INode()->GetTarget();
        auto next        = VFS::ResolvePath(parentEntry, target.Raw(), true)
                        .value_or(VFS::PathResolution{})
                        .Entry;

        if (!next) break;
        current = next;
    }

    auto iterator
        = [&](StringView name, loff_t offset, usize ino, usize type) -> bool
    {
        dirEntries.Push(name, offset, ino, type);
        return true;
    };
    (void)iterator;
    Delegate<bool(StringView, loff_t, usize, usize)> delegate;
    delegate.BindLambda(iterator);

    auto inode  = current->INode();
    auto result = inode->TraverseDirectories(current, delegate);
    CtosUnused(result);

    // . && ..
    StringView cwdPath = Process::Current()->CWD();
    auto       cwd     = VFS::ResolvePath(VFS::GetRootDirectoryEntry().Raw(), cwdPath)
                   .value_or(VFS::PathResolution{})
                   .Entry;
    if (!cwd) return true;

    auto stats = cwd->FollowMounts()->INode()->Stats();
    dirEntries.Push(".", 0, stats.st_ino, IF2DT(stats.st_mode));

    if (!cwd->Parent()) return true;

    stats = cwd->GetEffectiveParent()->INode()->Stats();
    dirEntries.Push("..", 0, stats.st_ino, IF2DT(stats.st_mode));
    return true;
}
