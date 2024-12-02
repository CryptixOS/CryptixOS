/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

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
}; // namespace MMIO
