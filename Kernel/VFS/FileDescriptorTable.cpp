/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptorTable.hpp>
#include <VFS/VFS.hpp>

FileDescriptorTable::FileDescriptorTable() {}

isize FileDescriptorTable::LowestFree(isize from) const
{
    isize fdNum = from < 0 ? 0 : from;
    while (m_Table.Contains(fdNum)) ++fdNum;
    return fdNum;
}
isize FileDescriptorTable::Insert(::Ref<FileDescriptor> fd, isize desired,
                                  bool closeOnExec)
{
    ScopedLock guard(m_Lock);
    isize      fdNum = desired >= 0 && !m_Table.Contains(desired)
                         ? desired
                         : LowestFree(0);
    m_Table[fdNum] = fd;
    if (closeOnExec) m_CloseOnExec[fdNum] = true;
    else m_CloseOnExec.Erase(fdNum);

    return fdNum;
}
isize FileDescriptorTable::Replace(::Ref<FileDescriptor> fd, isize desired,
                                   bool closeOnExec)
{
    ScopedLock guard(m_Lock);
    m_Table[desired] = fd;
    if (closeOnExec) m_CloseOnExec[desired] = true;
    else m_CloseOnExec.Erase(desired);

    return desired;
}
bool FileDescriptorTable::IsCloseOnExec(isize fd) const
{
    return m_CloseOnExec.Contains(fd);
}
bool FileDescriptorTable::SetCloseOnExec(isize fd, bool closeOnExec)
{
    ScopedLock guard(m_Lock);
    if (!m_Table.Contains(fd)) return false;

    if (closeOnExec) m_CloseOnExec[fd] = true;
    else m_CloseOnExec.Erase(fd);
    return true;
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
    m_CloseOnExec.Erase(fdNum);
    return 0;
}

void FileDescriptorTable::Clear()
{
    m_Table.Clear();
    m_CloseOnExec.Clear();
}
