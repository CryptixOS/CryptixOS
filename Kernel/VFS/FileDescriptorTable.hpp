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
#if CTOS_USE_PRISM_HASHMAP != 0
    using TableType = UnorderedMap<isize, FileDescriptor*>;
#else
    using TableType = std::unordered_map<isize, FileDescriptor*>;
#endif

  public:
    FileDescriptorTable() = default;

    i32         Insert(FileDescriptor* descriptor, i32 desired = -1);
    i32         Erase(i32 fdNum);

    void        OpenStdioStreams();
    void        Clear();

    inline bool IsValid(i32 fd) const
    {
#if CTOS_USE_PRISM_HASHMAP != 0
        return m_Table.Contains(fd);
#else
        return m_Table.contains(fd);
#endif
    }
    inline FileDescriptor* GetFd(i32 fd) const
    {
        if (!IsValid(fd)) return nullptr;

#if CTOS_USE_PRISM_HASHMAP != 0
        return m_Table.At(fd);
#else
        return m_Table.at(fd);
#endif
    }

    auto                    begin() { return m_Table.begin(); }
    auto                    end() { return m_Table.end(); }

    inline FileDescriptor*& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock    m_Lock;
    TableType   m_Table;

    Atomic<i32> m_NextIndex = 3;
};
