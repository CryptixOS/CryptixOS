/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

#define EFIAPI __attribute__((__ms_abi__))
#define EFI_IN
#define EFI_OUT
#define EFI_OPTIONAL
#define EFI_CONST

namespace EFI
{
    template <typename T, usize Alignment>
    struct AlignedType
    {
        alignas(Alignment) T Value;
    };

    using Boolean           = bool;
    constexpr Boolean False = false;
    constexpr Boolean True  = true;

    using SignedSize        = isize;
    using Size              = usize;
    using Int8              = i8;
    using Uint8             = u8;
    using Int16             = i16;
    using Uint16            = u16;
    using Int32             = i32;
    using Uint32            = u32;
    using Int64             = i64;
    using Uint64            = u64;
    using Int128            = __int128_t;
    using Uint128           = __uint128_t;
    using Char8             = unsigned char;
    using Char16            = u16;
    using Void              = void;
    using Guid              = Uint128;
    enum class Status : usize;
    using Handle      = Void*;
    using Event       = Void*;
    using Lba         = Uint64;
    using Tpl         = Size;
    using MacAddress  = Uint32;
    using Ipv4Address = Uint32;
    using Ipv6Address = Uint128;
    using IpAddress   = AlignedType<Uint128, 4>;
}; // namespace EFI

struct TableHeader
{
    u64 Signature;
    u32 Revision;
    u32 HeaderSize;
    u32 CRC32;
    u32 Reserved;
};
using EFI_HANDLE = void*;
