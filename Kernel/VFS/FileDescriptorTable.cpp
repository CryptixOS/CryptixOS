/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptorTable.hpp>
#include <VFS/VFS.hpp>

FileDescriptorTable::FileDescriptorTable() { m_NextIndex = 1; }
isize FileDescriptorTable::Insert(::Ref<FileDescriptor> fd, isize desired)
{
    ScopedLock guard(m_Lock);
    isize      fdNum = m_NextIndex;

    auto       found = m_Table.Find(desired);
    if (desired >= 0 && found == m_Table.end()) fdNum = desired;
    m_Table[fdNum] = fd;

    ++m_NextIndex;
    return fdNum;
}
isize FileDescriptorTable::Replace(::Ref<FileDescriptor> fd, isize desired)
{
    ScopedLock guard(m_Lock);
    m_Table[desired] = fd;

    return desired;
}
isize FileDescriptorTable::Erase(isize fdNum)
{
    ScopedLock            guard(m_Lock);
    ::Ref<FileDescriptor> fd = GetFd(fdNum);
    if (!fd) return_err(-1, EBADF);

    if (fd->DirectoryEntry()
        && fd->DirectoryEntry()->Name().Contains("ctl.sock"))
        LogTrace("Erasing auroractl.sock => {}", fd->DirectoryEntry()->Path());
    m_Table.Erase(fdNum);
    return 0;
}

void FileDescriptorTable::Clear()
{
    m_Table.Clear();
    m_NextIndex = 0;
}
