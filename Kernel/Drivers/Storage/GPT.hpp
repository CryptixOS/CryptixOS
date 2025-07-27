/*
 * Created by v1tr10l7 on 27.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>
#include <Prism/Core/Core.hpp>

namespace GPT
{
    inline constexpr usize HEADER_LENGTH    = 92;
    inline constexpr u64   HEADER_SIGNATURE = 0x5452'4150'2049'4645ull;

    enum class Attributes : u64
    {
        eImportant = Bit(0),
        eDontMount = Bit(1),
        eLegacy    = Bit(2),
    };
    constexpr Attributes operator|(Attributes lhs, Attributes rhs)
    {
        return static_cast<Attributes>(ToUnderlying(lhs) | ToUnderlying(rhs));
    }
    constexpr bool operator&(Attributes lhs, Attributes rhs)
    {
        return ToUnderlying(lhs) & ToUnderlying(rhs);
    }

    struct CTOS_PACKED Entry
    {
        u8         GUID[16];
        u8         UniqueGUID[16];
        u64        StartLBA;
        u64        EndLBA;
        Attributes Attributes;
        u8         Name[72];
    };

    struct CTOS_PACKED PartitionHeader
    {
        u32 Signature[2];
        u32 Revision;
        u32 HeaderSize;
        u32 Crc32Header;
        u32 Reserved;
        u64 CurrentLBA;
        u64 BackupLBA;
        u64 FirstBlock;
        u64 LastBlock;
        u8  DeviceGUID[16];
        u64 PartitionArrayLBA;
        u32 PartitionCount;
        u32 EntrySize;
        u32 PartitionArrayCRC32;
        u8  Padding[420];
    };
}; // namespace GPT
