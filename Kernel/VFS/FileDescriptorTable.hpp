/*
 * Created by v1tr10l7 on 27.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/UnorderedMap.hpp>
#include <VFS/FileDescriptor.hpp>

class FileDescriptorTable : public RefCounted<FileDescriptorTable>
{
    using TableType = UnorderedMap<isize, ::Ref<FileDescriptor>>;

  public:
    FileDescriptorTable();

    // Installs `descriptor` at `desired` if that slot is free, otherwise at
    // the lowest free number (never over an open fd). `closeOnExec` is the
    // per-fd FD_CLOEXEC flag; it is not shared with duplicates.
    isize       Insert(::Ref<FileDescriptor> descriptor, isize desired = -1,
                       bool closeOnExec = false);
    // dup2/dup3: installs at exactly `desired`, replacing what is there
    isize       Replace(::Ref<FileDescriptor> descriptor, isize desired,
                        bool closeOnExec = false);
    isize       Erase(isize fdNum);

    void        Clear();

    bool        IsCloseOnExec(isize fd) const;
    bool        SetCloseOnExec(isize fd, bool closeOnExec);

    inline bool IsValid(isize fd) const { return m_Table.Contains(fd); }
    inline ::Ref<FileDescriptor> GetFd(isize fd) const
    {
        if (!IsValid(fd)) return nullptr;

        return m_Table.At(fd);
    }

    auto                          begin() { return m_Table.begin(); }
    auto                          end() { return m_Table.end(); }

  private:
    Spinlock      m_Lock;
    TableType     m_Table;

    // fds with FD_CLOEXEC set
    UnorderedMap<isize, bool> m_CloseOnExec;

    isize         LowestFree(isize from) const;
};
