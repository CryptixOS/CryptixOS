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
  public:
    i32 Insert(FileDescriptor* descriptor)
    {
        i32 fd      = m_NextIndex++;
        m_Table[fd] = descriptor;

        return fd;
    }
    i32 Erase(i32 fd)
    {
        delete m_Table[fd];
        m_Table.erase(fd);

        // TODO(v1tr10l7): error handling
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

    using TableType = std::unordered_map<i32, FileDescriptor*>;
    TableType::iterator     begin() { return m_Table.begin(); }
    TableType::iterator     end() { return m_Table.end(); }

    inline FileDescriptor*& operator[](usize i) { return m_Table[i]; }

  private:
    TableType        m_Table;
    std::atomic<i32> m_NextIndex = 0;
};
