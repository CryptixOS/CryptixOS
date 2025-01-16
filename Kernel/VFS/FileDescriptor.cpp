/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptor.hpp>

#include <API/Posix/unistd.h>

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
