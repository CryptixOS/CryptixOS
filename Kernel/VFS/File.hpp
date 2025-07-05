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

class File
{
  public:
    virtual ~File() = default;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<const stat> Stat() const { return Error(ENOSYS); }
    virtual ErrorOr<isize>      Seek(i32 whence, off_t offset)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Truncate(off_t size) { return Error(ENOSYS); }

    virtual bool           IsCharDevice() const { return false; }
    virtual bool           IsFifo() const { return false; }
    virtual bool           IsDirectory() const { return false; }
    virtual bool           IsRegular() const { return false; }
    virtual bool           IsSymlink() const { return false; }
    virtual bool           IsSocket() const { return false; }

  private:
};
