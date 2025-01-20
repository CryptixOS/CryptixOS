/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/FileDescriptor.hpp>

#include <unordered_map>

class FileDescriptorTable
{
    using TableType = std::unordered_map<i32, FileDescriptor*>;

  public:
    FileDescriptorTable() = default;

    i32 Insert(FileDescriptor* descriptor, i32 desired = -1)
    {
        ScopedLock guard(m_Lock);
        i32        fdNum = m_NextIndex;
        if (desired >= 0 && m_Table.find(desired) == m_Table.end())
            fdNum = desired;
        m_Table[fdNum] = descriptor;

        ++m_NextIndex;
        return fdNum;
    }
    i32 Erase(i32 fdNum)
    {
        ScopedLock      guard(m_Lock);
        FileDescriptor* fd = GetFd(fdNum);
        if (!fd) return_err(-1, EBADF);

        delete fd;
        m_Table.erase(fdNum);

        return 0;
    }
    void Clear()
    {
        for (const auto& fd : m_Table) delete fd.second;

        m_Table.clear();
        m_NextIndex = 0;
    }

    inline bool IsValid(i32 fd) const
    {
        return m_Table.find(fd) != m_Table.end();
    }
    inline FileDescriptor* GetFd(i32 fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.at(fd);
    }

    TableType::iterator     begin() { return m_Table.begin(); }
    TableType::iterator     end() { return m_Table.end(); }

    inline FileDescriptor*& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock         m_Lock;
    TableType        m_Table;
    std::atomic<i32> m_NextIndex = 0;
};
