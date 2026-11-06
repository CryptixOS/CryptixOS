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

// set-sticky
constexpr usize S_ISVTX  = 0001000;
// set-group-id
constexpr usize S_ISGID  = 0002000;
// set-user-id
constexpr usize S_ISUID  = 0004000;
constexpr usize S_IFIFO  = 0010000;
constexpr usize S_IFCHR  = 0020000;
constexpr usize S_IFDIR  = 0040000;
constexpr usize S_IFBLK  = 0060000;
constexpr usize S_IFREG  = 0100000;
constexpr usize S_IFLNK  = 0120000;
constexpr usize S_IFSOCK = 0140000;
constexpr usize S_IFMT   = 0170000;

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

struct statx_timestamp
{
    i64 tv_sec;
    u32 tv_nsec;
    i32 __reserved;
};
struct statx
{
    /* 0x00 */
    /* What results were written [uncond] */
    u32                    stx_mask;

    /* Preferred general I/O size [uncond] */
    u32                    stx_blksize;

    /* Flags conveying information about the file [uncond] */
    u64                    stx_attributes;

    /* 0x10 */
    /* Number of hard links */
    u32                    stx_nlink;

    /* User ID of owner */
    u32                    stx_uid;

    /* Group ID of owner */
    u32                    stx_gid;

    /* File mode */
    u16                    stx_mode;
    u16                    __spare0[1];

    /* 0x20 */
    /* Inode number */
    u64                    stx_ino;

    /* File size */
    u64                    stx_size;

    /* Number of 512-byte blocks allocated */
    u64                    stx_blocks;

    /* Mask to show what's supported in stx_attributes */
    u64                    stx_attributes_mask;

    /* 0x40 */
    /* Last access time */
    struct statx_timestamp stx_atime;

    /* File creation time */
    struct statx_timestamp stx_btime;

    /* Last attribute change time */
    struct statx_timestamp stx_ctime;

    /* Last data modification time */
    struct statx_timestamp stx_mtime;

    /* 0x80 */
    /* Device ID of special file [if bdev/cdev] */
    u32                    stx_rdev_major;
    u32                    stx_rdev_minor;

    /* ID of device containing file [uncond] */
    u32                    stx_dev_major;
    u32                    stx_dev_minor;

    /* 0x90 */
    u64                    stx_mnt_id;

    /* Memory buffer alignment for direct I/O */
    u32                    stx_dio_mem_align;

    /* File offset alignment for direct I/O */
    u32                    stx_dio_offset_align;

    /* 0xa0 */
    /* Subvolume identifier */
    u64                    stx_subvol;

    /* Min atomic write unit in bytes */
    u32                    stx_atomic_write_unit_min;

    /* Max atomic write unit in bytes */
    u32                    stx_atomic_write_unit_max;

    /* 0xb0 */
    /* Max atomic write segment count */
    u32                    stx_atomic_write_segments_max;

    /* File offset alignment for direct I/O reads */
    u32                    stx_dio_read_offset_align;

    /* Optimised max atomic write unit in bytes */
    u32                    stx_atomic_write_unit_max_opt;
    u32                    __spare2[1];

    /* 0xc0 */
    u64                    __spare3[8]; /* Spare space for future expansion */

    /* 0x100 */
};

constexpr usize UTIME_NOW   = ((1l << 30) - 1l);
constexpr usize UTIME_OMIT  = ((1l << 30) - 2l);

constexpr usize SYMLOOP_MAX = 40zu;
