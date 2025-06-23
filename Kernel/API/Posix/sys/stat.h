/*
 * Created by v1tr10l7 on 23.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/time.h>
#include <Prism/Core/Types.hpp>

using dev_t              = u64;
using gid_t              = u32;
using ino_t              = u64;
using mode_t             = u32;
using nlink_t            = u64;
using off_t              = i64;
using loff_t             = i64;
using uid_t              = u32;
using blkcnt_t           = i64;
using blkcnt64_t         = i64;
using blksize_t          = i64;

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
    dev_t     st_dev;
    ino_t     st_ino;
    nlink_t   st_nlink;
    mode_t    st_mode = S_IFDIR;
    uid_t     st_uid;
    gid_t     st_gid;
    u32       __unused0;
    dev_t     st_rdev;
    off_t     st_size;

    blksize_t st_blksize;
    blkcnt_t  st_blocks;

    timespec  st_atim;
    timespec  st_mtim;
    timespec  st_ctim;

    i64       __unused1[3];
};

constexpr usize UTIME_NOW   = ((1l << 30) - 1l);
constexpr usize UTIME_OMIT  = ((1l << 30) - 2l);

constexpr usize SYMLOOP_MAX = 40zu;
