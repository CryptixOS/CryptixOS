/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>

struct dirent
{
    ino_t          d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[1024];
};

constexpr usize MAXNAMELEN    = 1024;
constexpr usize DIRENT_LENGTH = sizeof(dirent) - MAXNAMELEN;

constexpr usize IF2DT(mode_t mode) { return (mode & S_IFMT) >> 12; }
constexpr usize DT2IF(usize dirtype) { return dirtype << 12; }

constexpr usize DT_UNKNOWN = 0;
constexpr usize DT_FIFO    = 1;
constexpr usize DT_CHR     = 2;
constexpr usize DT_DIR     = 4;
constexpr usize DT_BLK     = 6;
constexpr usize DT_REG     = 8;
constexpr usize DT_LNK     = 10;
constexpr usize DT_SOCK    = 12;
constexpr usize DT_WHT     = 14;
