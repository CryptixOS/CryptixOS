/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/unistd.h>

#include <Scheduler/Process.hpp>

#include <Utility/Math.hpp>
#include <VFS/FileDescriptor.hpp>

void DirectoryEntries::Push(INode* node, std::string_view name)
{
    if (name.empty()) name = node->GetName();
    node            = node->Reduce(false);

    auto  reclen    = Math::AlignUp(DIRENT_LENGTH + name.length() + 1, 8);

    auto* entry     = reinterpret_cast<dirent*>(malloc(reclen));
    entry->d_ino    = node->GetStats().st_ino;
    entry->d_off    = reclen;
    entry->d_reclen = reclen;
    entry->d_type   = IF2DT(node->GetStats().st_mode);

    name.copy(entry->d_name, name.length());
    reinterpret_cast<char*>(entry->d_name)[name.length()] = 0;

    Entries.push_back(entry);
    Size += entry->d_reclen;
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
    --m_Description->RefCount;
    if (m_Description->RefCount == 0) delete m_Description;
}

ErrorOr<isize> FileDescriptor::Read(void* const outBuffer, usize count)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanRead()) return std::errno_t(EBADF);
    if (!GetNode()) return std::errno_t(ENOENT);
    if (GetNode()->IsDirectory()) return std::errno_t(EISDIR);

    // if (GetNode()->IsSocket())

    isize bytesRead
        = m_Description->Node->Read(outBuffer, m_Description->Offset, count);
    m_Description->Offset += bytesRead;

    return bytesRead;
}
ErrorOr<isize> FileDescriptor::Write(const void* data, isize bytes)
{
    ScopedLock guard(m_Description->Lock);

    if (!CanWrite()) return std::errno_t(EBADF);

    INode* node = GetNode();
    if (!node) return std::errno_t(ENOENT);
    isize offset       = m_Description->Offset;

    isize bytesWritten = node->Write(data, offset, bytes);
    m_Description->Offset += bytesWritten;

    return bytesWritten;
}

isize FileDescriptor::Seek(i32 whence, off_t offset)
{
    ScopedLock guard(m_Description->Lock);
    INode*     node = m_Description->Node;
    if (IsPipe() || node->IsSocket() || node->IsFifo())
        return std::errno_t(ESPIPE);

    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0) return std::errno_t(EINVAL);
            m_Description->Offset = offset;
            break;
        case SEEK_CUR:
        {
            isize newOffset = m_Description->Offset + offset;
            if (newOffset >= std::numeric_limits<off_t>::max())
                return std::errno_t(EOVERFLOW);
            if (newOffset < 0) return std::errno_t(EINVAL);
            m_Description->Offset += offset;
            break;
        }
        case SEEK_END:
        {
            usize size = m_Description->Node->GetStats().st_size;
            if (static_cast<usize>(m_Description->Offset) + size
                > std::numeric_limits<off_t>::max())
                return std::errno_t(EOVERFLOW);
            m_Description->Offset
                = m_Description->Node->GetStats().st_size + offset;
            break;
        }

        default: return std::errno_t(EINVAL);
    };

    return m_Description->Offset;
}

[[clang::no_sanitize("alignment")]]
ErrorOr<i32> FileDescriptor::GetDirEntries(dirent* const out, usize maxSize)
{
    ScopedLock guard(m_Description->Lock);

    INode*     node = GetNode();
    if (!node) return std::errno_t(ENOENT);
    if (!node->IsDirectory()) return std::errno_t(ENOTDIR);

    DirectoryEntries& dirEntries = GetDirEntries();
    if (dirEntries.IsEmpty()) GenerateDirEntries();
    if (dirEntries.IsEmpty()) return 0;

    usize length = 0;
    for (const auto& entry : dirEntries) length += entry->d_reclen;
    length = std::min(maxSize, length);

    if (dirEntries.Front()->d_reclen > maxSize) return std::errno_t(EINVAL);

    usize bytes  = 0;
    usize offset = 0;

    while (offset < length)
    {
        bytes = offset;
        offset += dirEntries.CopyAndPop(reinterpret_cast<u8*>(out) + offset);
    }

    if (dirEntries.IsEmpty()) bytes = true;

    return bytes;
}
bool FileDescriptor::GenerateDirEntries()
{
    DebugWarnIf(m_Description->Lock.Test(),
                "FileDescriptor::GenerateDirEntries called without "
                "m_Description->Lock being acquired!");
    INode* node = GetNode();
    if (!node || !node->IsDirectory()) return std::errno_t(ENOTDIR);

    DirectoryEntries& dirEntries = GetDirEntries();
    dirEntries.Clear();

    node = m_Description->Node->Reduce(true, true);
    for (const auto [name, child] : node->GetChildren()) dirEntries.Push(child);

    // . && ..
    INode* cwd = Process::GetCurrent()->GetCWD();
    if (!cwd) return true;

    dirEntries.Push(cwd, ".");
    if (!cwd->GetParent()) return true;
    dirEntries.Push(cwd->GetParent(), "..");
    return true;
}
