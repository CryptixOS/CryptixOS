/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>

#include <Library/UserBuffer.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Core/Error.hpp>

class INode;
class DirectoryEntry;
class File
{
  public:
    File() = default;
    explicit File(class INode* inode);

    virtual ~File() = default;

    virtual class INode*   INode() const;

    virtual usize          Size() const;

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes);
    virtual ErrorOr<isize> Write(const void* src, off_t offset, usize bytes);
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1);
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1);
    virtual ErrorOr<const stat> Stat() const;
    virtual ErrorOr<isize>      Seek(i32 whence, off_t offset)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Truncate(off_t size);

    virtual bool           IsCharDevice() const { return false; }
    virtual bool           IsBlockDevice() const { return false; }
    virtual bool           IsFifo() const { return false; }
    virtual bool           IsDirectory() const { return false; }
    virtual bool           IsRegular() const { return false; }
    virtual bool           IsSymlink() const { return false; }
    virtual bool           IsSocket() const { return false; }

  private:
    Spinlock                    m_Lock;
    CTOS_UNUSED DirectoryEntry* m_DirectoryEntry = nullptr;
    CTOS_UNUSED class INode*    m_INode          = nullptr;
};
