/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

namespace Prism::Math
{
    inline constexpr usize AlignDown(usize value, usize alignment)
    {
        return value & ~(alignment - 1);
    }
    inline constexpr usize AlignUp(usize value, usize alignment)
    {
        return AlignDown(value + alignment - 1, alignment);
    }
    inline constexpr auto DivRoundUp(auto value, auto alignment)
    {
        return AlignDown(value + alignment - 1, alignment) / alignment;
    }
    inline constexpr bool IsPowerOfTwo(usize value)
    {
        return value != 0 && !(value & (value - 1));
    }

    template <typename T>
    inline T PowerOf(T base, T exponent)
    {
        T invert  = 0;

        T powMult = 1;

        if ((invert = (exponent < 0))) exponent = -exponent;

        for (T x = 0; x < exponent; x++) powMult *= base;

        return (invert) ? 1 / powMult : powMult;
    }
} // namespace Prism::Math

CryptixNameSpace
