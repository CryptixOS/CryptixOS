/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/unistd.h>
#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Utility/Math.hpp>
#include <VFS/FileDescriptor.hpp>

void DirectoryEntries::Push(INode* node, StringView name)
{
    if (name.Empty()) name = node->GetName();
    node = node->Reduce(false);

    auto reclen
        = Math::AlignUp(DIRENT_LENGTH + name.Size() + 1, alignof(dirent));

    auto* entry     = reinterpret_cast<dirent*>(malloc(reclen));
    entry->d_ino    = node->GetStats().st_ino;
    entry->d_off    = reclen;
    entry->d_reclen = reclen;
    entry->d_type   = IF2DT(node->GetStats().st_mode);

    name.Copy(entry->d_name, name.Size() + 1);
    reinterpret_cast<char*>(entry->d_name)[name.Size()] = 0;

    Entries.push_back(entry);
    Size += entry->d_reclen;
}
usize DirectoryEntries::CopyAndPop(u8* out, usize capacity)
{
    if (Entries.empty()) return 0;

    auto  entry     = Entries.front();
    usize entrySize = entry->d_reclen;
    if (capacity < entrySize) return 0;

    Entries.pop_front();
    if (!entry || !(char*)entry->d_name) return 0;

    std::memcpy(out, entry, entry->d_reclen);
    delete entry;

    return entrySize;
}

FileDescriptor::FileDescriptor(INode* node, i32 flags, FileAccessMode accMode)
{
    m_Description = new FileDescription;
    m_Description->IncRefCount();
    m_Description->Node  = node;

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

ErrorOr<isize> FileDescriptor::Read(void* const outBuffer, usize count)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanRead()) return Error(EBADF);
    if (!GetNode()) return Error(ENOENT);
    if (GetNode()->IsDirectory()) return Error(EISDIR);

    if (WouldBlock())
    {
        if (IsNonBlocking()) return Error(IsSocket() ? EWOULDBLOCK : EAGAIN);
        Thread* current = CPU::GetCurrentThread();
        Scheduler::Block(current);

        if (current->WasInterrupted()) return Error(EINTR);
    }

    isize bytesRead
        = m_Description->Node->Read(outBuffer, m_Description->Offset, count);
    m_Description->Offset += bytesRead;

    return bytesRead;
}
ErrorOr<isize> FileDescriptor::Write(const void* data, isize bytes)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanWrite()) return Error(EBADF);

    INode* node = GetNode();
    if (!node) return Error(ENOENT);
    isize offset       = m_Description->Offset;

    isize bytesWritten = node->Write(data, offset, bytes);
    m_Description->Offset += bytesWritten;

    return bytesWritten;
}
ErrorOr<const stat*> FileDescriptor::Stat() const
{
    INode* node = GetNode();
    if (!node) return std::unexpected(Error(ENOENT));

    return &node->GetStats();
}

ErrorOr<isize> FileDescriptor::Seek(i32 whence, off_t offset)
{
    ScopedLock guard(m_Description->Lock);
    INode*     node = m_Description->Node;
    if (IsPipe() || node->IsSocket() || node->IsFifo()) return Error(ESPIPE);

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
            usize size = m_Description->Node->GetStats().st_size;
            if (static_cast<usize>(m_Description->Offset) + size
                > std::numeric_limits<off_t>::max())
                return Error(EOVERFLOW);
            m_Description->Offset
                = m_Description->Node->GetStats().st_size + offset;
            break;
        }

        default: return Error(EINVAL);
    };

    return m_Description->Offset;
}

ErrorOr<isize> FileDescriptor::Truncate(off_t size)
{
    if (m_Description->Node->IsDirectory()) return Error(EISDIR);
    if (!(m_Description->AccessMode & FileAccessMode::eWrite)
        || !m_Description->Node->IsRegular())
        return Error(EBADF);

    return m_Description->Node->Truncate(size);
}

[[clang::no_sanitize("alignment")]] ErrorOr<i32>
FileDescriptor::GetDirEntries(dirent* const out, usize maxSize)
{
    ScopedLock guard(m_Description->Lock);

    INode*     node = GetNode();
    if (!node) return Error(ENOENT);
    if (!node->IsDirectory()) return Error(ENOTDIR);

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
    INode*            node       = GetNode();

    DirectoryEntries& dirEntries = GetDirEntries();
    dirEntries.Clear();

    node = m_Description->Node->Reduce(true, true);
    for (const auto [name, child] : node->GetChildren())
    {
        if (name.Empty()) continue;
        dirEntries.Push(child);
    }

    // . && ..
    StringView cwdPath = Process::GetCurrent()->GetCWD();
    auto       cwd     = VFS::ResolvePath(VFS::GetRootNode(), cwdPath).Node;
    if (!cwd) return true;

    dirEntries.Push(cwd, ".");
    if (!cwd->GetParent()) return true;
    dirEntries.Push(cwd->GetParent(), "..");
    return true;
}
