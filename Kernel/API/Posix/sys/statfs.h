/*
 * Created by v1tr10l7 on 20.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

using __fsword_t = long;
using fsblkcnt_t = u64;
using fsfilcnt_t = u64;
using fsid_t     = u64;

struct statfs
{
    // Type of filesystem
    __fsword_t f_type;
    // Optimal transfer block size
    __fsword_t f_bsize;
    // Total data blocks in filesystem
    fsblkcnt_t f_blocks;
    // Free blocks in filesystem
    fsblkcnt_t f_bfree;
    // Free blocks available to unprivileged user
    fsblkcnt_t f_bavail;

    // Total inodes in filesystem
    fsfilcnt_t f_files;
    // Free inodes in filesystem
    fsfilcnt_t f_ffree;
    // Filesystem ID
    fsid_t     f_fsid;
    // Maximum length of filenames
    __fsword_t f_namelen;
    // Frgment size
    __fsword_t f_frsize;
    // Mount flags of filesystem
    __fsword_t f_flags;

    // Padding bytes reserved for future use
    __fsword_t f_spare[4];
};
