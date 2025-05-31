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

#include <VFS/INode.hpp>

class FileHandle
{
  public:
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1)
        = 0;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1)
        = 0;
};
