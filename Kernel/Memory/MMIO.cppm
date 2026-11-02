/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

export module Cryptix:Memory.MMIO;

export import :Debug.Assertions;

namespace MMIO
{
    export template <UnsignedIntegral T>
    inline T Read(Pointer address)
    {
        volatile T* reg = address.As<volatile T>();

        return *reg;
    }
    export template <UnsignedIntegral T>
    inline void Write(Pointer address, T value)
    {
        volatile T* reg = address.As<volatile T>();

        *reg            = value;
    }

    export inline usize Read(Pointer address, u8 accessSize)
    {
        switch (accessSize)
        {
            case 1: return Read<u8>(address);
            case 2: return Read<u16>(address);
            case 3: return Read<u32>(address);
            case 4: return Read<u64>(address);

            default: break;
        }

        AssertNotReached();
    }
}; // namespace MMIO
