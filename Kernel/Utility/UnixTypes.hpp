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
    typedef __SIZE_TYPE__ ssize_t;
#undef unsigned
#pragma clang diagnostic pop

    typedef int64_t      blkcnt_t;
    typedef int64_t      blksize_t;
    typedef uint64_t     clock_t;
    typedef int          clockid_t;
    typedef uint64_t     dev_t;
    typedef uint64_t     fsblkcnt_t;
    typedef uint64_t     fsfilcnt_t;
    typedef uint32_t     gid_t;
    typedef int32_t      id_t;
    typedef uint64_t     ino_t;
    typedef uint64_t     key_t;
    typedef unsigned int mode_t;
    typedef uint64_t     nlink_t;
    typedef int64_t      off_t;
    typedef i64          pid_t;
    typedef uint32_t     uid_t;
    typedef int64_t      time_t;

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
