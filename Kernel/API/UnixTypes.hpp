/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stdint.h>

#include "Common.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define unsigned signed
    using ssize_t = __SIZE_TYPE__;
#undef unsigned
#pragma clang diagnostic pop

    using blkcnt_t   = int64_t;
    using blksize_t  = int64_t;
    using clock_t    = uint64_t;
    using clockid_t  = int;
    using dev_t      = uint64_t;
    using fsblkcnt_t = uint64_t;
    using fsfilcnt_t = uint64_t;
    using gid_t      = uint32_t;
    using id_t       = int32_t;
    using ino_t      = uint64_t;
    using key_t      = uint64_t;
    using mode_t     = unsigned int;
    using nlink_t    = uint64_t;
    using off_t      = int64_t;
    using pid_t      = i64;
    using uid_t      = uint32_t;
    using time_t     = int64_t;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct timespec
    {
        time_t tv_sec;
        long   tv_nsec;
    };

#ifdef __cplusplus
}
#endif

#define S_IFMT      0170000
#define S_IFBLK     0060000
#define S_IFCHR     0020000
#define S_IFIFO     0010000
#define S_IFREG     0100000
#define S_IFDIR     0040000
#define S_IFLNK     0120000
#define S_IFSOCK    0140000

#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#ifdef __cplusplus
extern "C"
{
#endif

    struct stat
    {
        dev_t           st_dev;
        ino_t           st_ino;
        mode_t          st_mode;
        nlink_t         st_nlink;
        uid_t           st_uid;
        gid_t           st_gid;
        dev_t           st_rdev;
        off_t           st_size;

        blksize_t       st_blksize;
        blkcnt_t        st_blocks;

        struct timespec st_atim;
        struct timespec st_mtim;
        struct timespec st_ctim;
    };

#ifdef __cplusplus
}
#endif

#define SYMLOOP_MAX 40
