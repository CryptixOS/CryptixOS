/*
 * Created by v1tr10l7 on 19.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr usize O_RDONLY    = 0x0000;
constexpr usize O_WRONLY    = 0x0001;
constexpr usize O_RDWR      = 0x0002;

constexpr usize O_CREAT     = 0x0100;
constexpr usize O_EXCL      = 0x0200;
constexpr usize O_NOCTTY    = 0x0400;
constexpr usize O_TRUNC     = 0x1000;
constexpr usize O_APPEND    = 0x2000;
constexpr usize O_NONBLOCK  = 0x4000;
constexpr usize O_NDELAY    = O_NONBLOCK;
constexpr usize O_DSYNC     = 010000;
constexpr usize O_ASYNC     = 020000;
constexpr usize O_DIRECT    = 040000;
constexpr usize O_LARGEFILE = 0100000;
constexpr usize O_DIRECTORY = 0200000;
constexpr usize O_NOFOLLOW  = 0400000;
constexpr usize O_NOATIME   = 01000000;
constexpr usize O_CLOEXEC   = 02000000;
constexpr usize O_SYNC      = 04010000;
constexpr usize O_RSYNC     = 04010000;
constexpr usize O_PATH      = 010000000;
constexpr usize O_TMPFILE   = 020000000;
constexpr usize O_EXEC      = O_PATH;
constexpr usize O_SEARCH    = O_PATH;
constexpr usize FASYNC      = 00020000;

constexpr usize O_ACCMODE   = O_PATH | 03;

constexpr usize __O_SYNC    = 020000000;
constexpr usize __O_TMPFILE = 0100000000;

constexpr usize VALID_OPEN_FLAGS
    = O_RDONLY | O_WRONLY | O_RDWR | O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC
    | O_APPEND | O_NDELAY | O_NONBLOCK | __O_SYNC | O_DSYNC | FASYNC | O_DIRECT
    | O_LARGEFILE | O_DIRECTORY | O_NOFOLLOW | O_NOATIME | O_CLOEXEC | O_PATH
    | __O_TMPFILE;

constexpr usize F_DUPFD             = 0;
constexpr usize F_GETFD             = 1;
constexpr usize F_SETFD             = 2;
constexpr usize F_GETFL             = 3;
constexpr usize F_SETFL             = 4;
constexpr usize F_DUPFD_CLOEXEC     = 1030;

constexpr usize R_OK                = 4; /* Test for read permission.  */
constexpr usize W_OK                = 2; /* Test for write permission.  */
constexpr usize X_OK                = 1; /* Test for execute permission.  */
constexpr usize F_OK                = 0; /* Test for existence.  */

constexpr isize AT_FDCWD            = -100;
constexpr isize AT_SYMLINK_NOFOLLOW = 0x100;
constexpr isize AT_REMOVEDIR        = 0x200;
constexpr isize AT_SYMLINK_FOLLOW   = 0x400;
constexpr isize AT_NO_AUTOMOUNT     = 0x800;
constexpr isize AT_EMPTY_PATH       = 0x1000;
constexpr isize AT_RECURSIVE        = 0x8000;
constexpr isize AT_EACCESS          = 0x200;

// owner
// read
constexpr usize S_IREAD             = 0400;
// write
constexpr usize S_IWRITE            = 0200;
// exec
constexpr usize S_IEXEC             = 0100;

// rwx
constexpr usize S_IRWXU             = 00700;
// read
constexpr usize S_IRUSR             = S_IREAD;
// write
constexpr usize S_IWUSR             = S_IWRITE;
// exec
constexpr usize S_IXUSR             = S_IEXEC;

// group
// rwx
constexpr usize S_IRWXG             = 00070;
// read
constexpr usize S_IRGRP             = 00040;
// write
constexpr usize S_IWGRP             = 00020;
// exec
constexpr usize S_IXGRP             = 00010;

// others
// rwx
constexpr usize S_IRWXO             = 00007;
// read
constexpr usize S_IROTH             = 00004;
// write
constexpr usize S_IWOTH             = 00002;
// exec
constexpr usize S_IXOTH             = 00001;
