/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/UnorderedMap.hpp>
#include <VFS/FileDescriptor.hpp>

class FileDescriptorTable
{
    using TableType = UnorderedMap<i32, FileDescriptor*>;

  public:
    FileDescriptorTable() = default;

    i32         Insert(FileDescriptor* descriptor, i32 desired = -1);
    i32         Erase(i32 fdNum);

    void        OpenStdioStreams();
    void        Clear();

    inline bool IsValid(i32 fd) const { return m_Table.Contains(fd); }
    inline FileDescriptor* GetFd(i32 fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.At(fd);
    }

    auto                    begin() { return m_Table.begin(); }
    auto                    end() { return m_Table.end(); }

    inline FileDescriptor*& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock    m_Lock;
    TableType   m_Table;
    Atomic<i32> m_NextIndex = 3;
};
