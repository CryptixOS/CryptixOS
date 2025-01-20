/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptor.hpp>

#include <API/Posix/unistd.h>
#include <Utility/Math.hpp>

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

bool FileDescriptor::GenerateDirEntries()
{
    ScopedLock guard(m_Description->Lock);
    if (!m_Description->Node || !m_Description->Node->IsDirectory())
        return std::errno_t(ENOTDIR);

    auto node = m_Description->Node->Reduce(true, true);

    m_Description->DirEntries.clear();
    for (const auto [name, child] : node->GetChildren())
    {
        auto  current   = child->Reduce(false);
        auto  reclen    = Math::AlignUp(DIRENT_LENGTH + name.length() + 1, 8);

        auto* entry     = reinterpret_cast<dirent*>(malloc(reclen));
        entry->d_ino    = current->GetStats().st_ino;
        entry->d_off    = reclen;
        entry->d_reclen = reclen;
        entry->d_type   = IF2DT(current->GetStats().st_mode);

        name.copy(entry->d_name, name.length());
        reinterpret_cast<char*>(entry->d_name)[name.length()] = 0;

        m_Description->DirEntries.push_back(entry);
    }

    return true;
    for (std::string_view name : {".", ".."})
    {
        auto current = node;
        if (name != "." && node->GetName() != "/") current = node->GetParent();

        auto  reclen    = DIRENT_LENGTH + name.length() + 1;

        auto* entry     = reinterpret_cast<dirent*>(malloc(reclen));
        entry->d_ino    = current->GetStats().st_ino;
        entry->d_off    = reclen;
        entry->d_reclen = reclen;
        entry->d_type   = IF2DT(current->GetStats().st_mode);

        name.copy(entry->d_name, name.length());
        entry->d_name[name.length()] = 0;

        m_Description->DirEntries.push_back(entry);
    }

    return true;
}
