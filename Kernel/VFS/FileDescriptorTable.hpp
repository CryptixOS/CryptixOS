/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/UnorderedMap.hpp>
#include <VFS/FileDescriptor.hpp>

class FileDescriptorTable : public RefCounted
{
    using TableType = UnorderedMap<isize, ::Ref<FileDescriptor>>;

  public:
    FileDescriptorTable();

    isize       Insert(::Ref<FileDescriptor> descriptor, isize desired = -1);
    isize       Replace(::Ref<FileDescriptor> descriptor, isize desired);
    isize       Erase(isize fdNum);

    void        Clear();

    inline bool IsValid(isize fd) const { return m_Table.Contains(fd); }
    inline ::Ref<FileDescriptor> GetFd(isize fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.At(fd);
    }

    auto                          begin() { return m_Table.begin(); }
    auto                          end() { return m_Table.end(); }

    inline ::Ref<FileDescriptor>& operator[](usize i) { return m_Table[i]; }

  private:
    Spinlock      m_Lock;
    TableType     m_Table;

    Atomic<isize> m_NextIndex = 0;
};
