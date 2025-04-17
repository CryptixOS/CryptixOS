/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Debug/Assertions.hpp>

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

#include <concepts>

namespace MMIO
{
    template <std::unsigned_integral T>
    inline static T Read(Pointer address)
    {
        volatile T* reg = address.As<volatile T>();

        return *reg;
    }
    template <std::unsigned_integral T>
    inline static void Write(Pointer address, T value)
    {
        volatile T* reg = address.As<volatile T>();

        *reg            = value;
    }

    inline static usize Read(Pointer address, u8 accessSize)
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
