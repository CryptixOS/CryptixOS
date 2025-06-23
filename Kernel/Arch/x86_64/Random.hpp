/*
 * Created by v1tr10l7 on 07.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace Arch
{
    inline constexpr bool rdseed(u64& value)
    {
        bool status = false;
        __asm__ volatile("rdseed %[out]\n\t" : "=c"(status), [out] "=r"(value));

        return status;
    }
    inline constexpr bool rdrand(u64& value)
    {
        bool status = false;
        u32  retry  = 10;

        do {
            __asm__ volatile("rdrand %[out]\n\t"
                             : "=c"(status), [out] "=r"(value));
        } while (--retry && !status);

        return status;
    }
}; // namespace Arch
