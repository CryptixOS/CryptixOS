/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Error.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringView.hpp>

namespace Ustar
{
    struct [[gnu::packed]] FileHeader
    {
        char FileName[100];
        char Mode[8];
        char Uid[8];
        char Gid[8];
        char FileSize[12];
        char MTime[12];
        char Checksum[8];
        char Type;
        char LinkName[100];
        char Signature[6];
        char Version[2];
        char UserName[32];
        char GroupName[32];
        char DeviceMajor[8];
        char DeviceMinor[8];
        char FilenamePrefix[155];
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

    bool                 Validate(Pointer address);
    ErrorOr<void>        Load(Pointer address, usize size);
} // namespace Ustar
