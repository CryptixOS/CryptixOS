/*
 * Created by v1tr10l7 on 19.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

constexpr usize O_RDONLY            = 0x0000;
constexpr usize O_WRONLY            = 0x0001;
constexpr usize O_RDWR              = 0x0002;
constexpr usize O_APPEND            = 0x0008;
constexpr usize O_CREAT             = 0x0200;
constexpr usize O_TRUNC             = 0x0400;
constexpr usize O_EXCL              = 0x0800;
constexpr usize O_NONBLOCK          = 0x4000;
constexpr usize O_ASYNC             = 020000;
constexpr usize O_LARGEFILE         = 0100000;
constexpr usize O_DIRECTORY         = 0200000;
constexpr usize O_NOFOLLOW          = 0400000;

constexpr usize O_ACCMODE           = (O_RDONLY | O_WRONLY | O_RDWR);

constexpr usize F_DUPFD             = 0;
constexpr usize F_GETFL             = 3;
constexpr usize F_SETFL             = 4;

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

#define O_ACCMODE 0003
#define O_RDONLY  00
#define O_WRONLY  01
#define O_RDWR    02

#define S_ISUID   04000 /* Set user ID on execution.  */
#define S_ISGID   02000 /* Set group ID on execution.  */
#define S_ISVTX   01000 /* Save swapped text after use (sticky).  */
#define S_IREAD   0400  /* Read by owner.  */
#define S_IWRITE  0200  /* Write by owner.  */
#define S_IEXEC   0100  /* Execute by owner.  */

#define S_IRWXU   00700
#define S_IRUSR   00400
#define S_IWUSR   00200
#define S_IXUSR   00100

#define S_IRWXG   00070
#define S_IRGRP   00040
#define S_IWGRP   00020
#define S_IXGRP   00010

#define S_IRWXO   00007
#define S_IROTH   00004
#define S_IWOTH   00002
#define S_IXOTH   00001
