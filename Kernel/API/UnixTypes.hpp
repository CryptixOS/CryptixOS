/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/time.h>

#include <Prism/Core/Types.hpp>

using ssize_t            = isize;

using blkcnt_t           = long;
using blksize_t          = long;
using dev_t              = unsigned long;
// using fsblkcnt_t = u64;
// using fsfilcnt_t = u64;
using gid_t              = unsigned int;
// using id_t       = i64;
using ino_t              = unsigned long;
// using key_t      = u64;
using mode_t             = unsigned int;
using nlink_t            = unsigned long;
using off_t              = long;
using pid_t              = int;
using tid_t              = int;
using uid_t              = unsigned int;

constexpr usize S_IFMT   = 0170000;
constexpr usize S_IFBLK  = 0060000;
constexpr usize S_IFCHR  = 0020000;
constexpr usize S_IFIFO  = 0010000;
constexpr usize S_IFREG  = 0100000;
constexpr usize S_IFDIR  = 0040000;
constexpr usize S_IFLNK  = 0120000;
constexpr usize S_IFSOCK = 0140000;

constexpr bool  S_ISBLK(mode_t mode) { return (mode & S_IFMT) == S_IFBLK; }
constexpr bool  S_ISCHR(mode_t mode) { return (mode & S_IFMT) == S_IFCHR; }
constexpr bool  S_ISDIR(mode_t mode) { return (mode & S_IFMT) == S_IFDIR; }
constexpr bool  S_ISFIFO(mode_t mode) { return (mode & S_IFMT) == S_IFIFO; }
constexpr bool  S_ISREG(mode_t mode) { return (mode & S_IFMT) == S_IFREG; }
constexpr bool  S_ISLNK(mode_t mode) { return (mode & S_IFMT) == S_IFLNK; }
constexpr bool  S_ISSOCK(mode_t mode) { return (mode & S_IFMT) == S_IFSOCK; }

struct stat
{
    dev_t        st_dev;
    ino_t        st_ino;
    nlink_t      st_nlink;
    mode_t       st_mode = S_IFDIR;
    uid_t        st_uid;
    gid_t        st_gid;
    unsigned int __unused0;
    dev_t        st_rdev;
    off_t        st_size;

    blksize_t    st_blksize;
    blkcnt_t     st_blocks;

    timespec     st_atim;
    timespec     st_mtim;
    timespec     st_ctim;

    long         __unused1[3];
};

constexpr usize SYMLOOP_MAX = 40zu;
