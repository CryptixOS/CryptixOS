/*
 * Created by v1tr10l7 on 15.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/File.hpp>
#include <VFS/INode.hpp>

File::File(class INode* inode)
    : m_INode(inode)
{
}

class INode* File::INode() const { return m_INode; }
usize File::Size() const { return m_INode ? m_INode->Stats().st_size : 0; }

ErrorOr<isize> File::Read(void* dest, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Read(dest, offset, bytes);
}
ErrorOr<isize> File::Write(const void* src, off_t offset, usize bytes)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Write(src, offset, bytes);
}
ErrorOr<isize> File::Read(const UserBuffer& out, usize count, isize offset)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Read(out.Raw(), offset, count);
}

ErrorOr<isize> File::Write(const UserBuffer& in, usize count, isize offset)
{
    ScopedLock guard(m_Lock);
    if (m_INode->IsDirectory()) return Error(EISDIR);

    return m_INode->Write(in.Raw(), offset, count);
}

ErrorOr<const stat> File::Stat() const
{
    if (!m_INode) return Error(ENOENT);

    return m_INode->Stats();
}

ErrorOr<isize> File::Truncate(off_t size) { return m_INode->Truncate(size); }
