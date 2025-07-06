/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/FileDescriptorTable.hpp>
#include <VFS/VFS.hpp>

i32 FileDescriptorTable::Insert(FileDescriptor* fd, i32 desired)
{
    ScopedLock guard(m_Lock);
    i32        fdNum = m_NextIndex;

#if CTOS_USE_PRISM_HASHMAP != 0
    auto found = m_Table.Find(desired);
#else
    auto found = m_Table.find(desired);
#endif
    if (desired >= 0 && found == m_Table.end()) fdNum = desired;
    m_Table[fdNum] = fd;

    ++m_NextIndex;
    return fdNum;
}
i32 FileDescriptorTable::Erase(i32 fdNum)
{
    ScopedLock      guard(m_Lock);
    FileDescriptor* fd = GetFd(fdNum);
    if (!fd) return_err(-1, EBADF);

    delete fd;

#if CTOS_USE_PRISM_HASHMAP != 0
    m_Table.Erase(fdNum);
#else
    m_Table.erase(fdNum);
#endif

    return 0;
}

void FileDescriptorTable::OpenStdioStreams()
{
    // FIXME(v1tr10l7): Should we verify whether stdio fds are already open?
    Ref ttyNode
        = VFS::ResolvePath(VFS::GetRootDirectoryEntry().Raw(), "/dev/tty")
              .value()
              .Entry;

    Insert(new FileDescriptor(ttyNode.Raw(), 0, FileAccessMode::eRead), 0);
    Insert(new FileDescriptor(ttyNode.Raw(), 0, FileAccessMode::eWrite), 1);
    Insert(new FileDescriptor(ttyNode.Raw(), 0, FileAccessMode::eWrite), 2);
}
void FileDescriptorTable::Clear()
{
    for (const auto& [i, fd] : m_Table) delete fd;

#if CTOS_USE_PRISM_HASHMAP != 0
    m_Table.Clear();
#else
    m_Table.clear();
#endif
    m_NextIndex = 3;
}
