/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>

namespace Ustar
{
    struct [[gnu::packed]] FileHeader
    {
        char filename[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char fileSize[12];
        char mtime[12];
        char checksum[8];
        char type;
        char linkName[100];
        char signature[6];
        char version[2];
        char userName[32];
        char groupName[32];
        char deviceMajor[8];
        char deviceMinor[8];
        char filenamePrefix[155];
    };

    constexpr StringView MAGIC                      = "ustar";
    constexpr const u8   MAGIC_LENGTH               = 6;

    // NOTE: Both '0' and '\0' mean the same file type
    constexpr const u8   FILE_TYPE_NORMAL           = '\0';
    constexpr const u8   FILE_TYPE_NORMAL_          = '0';
    constexpr const u8   FILE_TYPE_HARD_LINK        = '1';
    constexpr const u8   FILE_TYPE_SYMLINK          = '2';
    constexpr const u8   FILE_TYPE_CHARACTER_DEVICE = '3';
    constexpr const u8   FILE_TYPE_BLOCK_DEVICE     = '4';
    constexpr const u8   FILE_TYPE_DIRECTORY        = '5';
    constexpr const u8   FILE_TYPE_FIFO             = '6';
    constexpr const u8   FILE_TYPE_CONTIGUOUS       = '7';

    bool                 Validate(uintptr_t address);
    void                 Load(uintptr_t address);
} // namespace Ustar
