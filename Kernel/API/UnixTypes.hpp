/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

using ssize_t    = isize;

using blkcnt_t   = isize;
using blksize_t  = isize;
using clock_t    = i32;
using clockid_t  = i64;
using dev_t      = usize;
using fsblkcnt_t = u64;
using fsfilcnt_t = u64;
using gid_t      = i32;
using id_t       = i64;
using ino_t      = usize;
using key_t      = u64;
using mode_t     = u32;
using nlink_t    = usize;
using off_t      = isize;
using pid_t      = i32;
using tid_t      = i32;
using uid_t      = u32;
using time_t     = isize;

struct timespec
{
    time_t tv_sec;
    long   tv_nsec;
};

constexpr usize S_IFMT   = 0170000;
constexpr usize S_IFBLK  = 0060000;
constexpr usize S_IFCHR  = 0020000;
constexpr usize S_IFIFO  = 0010000;
constexpr usize S_IFREG  = 0100000;
constexpr usize S_IFDIR  = 0040000;
constexpr usize S_IFLNK  = 0120000;
constexpr usize S_IFSOCK = 0140000;

#define S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)
#define S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)

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
