/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptor.hpp>

#include <API/Posix/unistd.h>
#include <Utility/Math.hpp>

isize FileDescriptor::Read(void* const outBuffer, usize count)
{
    ScopedLock guard(m_Lock);

    isize      bytesRead = m_Node->Read(outBuffer, m_Offset, count);
    m_Offset += bytesRead;

    m_Node->UpdateATime();

    return bytesRead;
}
isize FileDescriptor::Seek(i32 whence, off_t offset)
{
    switch (whence)
    {
        case SEEK_SET: m_Offset = offset; break;
        case SEEK_CUR:
            if (usize(m_Offset) + usize(offset) > sizeof(off_t))
                return_err(-1, EOVERFLOW);
            m_Offset += offset;
            break;
        case SEEK_END:
        {
            usize size = m_Node->GetStats().st_size;
            if (usize(m_Offset) + size > sizeof(off_t))
                return_err(-1, EOVERFLOW);
            m_Offset = m_Node->GetStats().st_size + offset;
            break;
        }

        default: return_err(-1, EINVAL);
    };

    return m_Offset;
}

bool FileDescriptor::GenerateDirEntries()
{
    ScopedLock guard(m_Lock);
    if (!m_Node || !m_Node->IsDirectory()) return_err(false, ENOTDIR);

    auto node = m_Node->Reduce(true, true);

    m_DirEntries.clear();
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

        m_DirEntries.push_back(entry);
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

        m_DirEntries.push_back(entry);
    }

    return true;
}
