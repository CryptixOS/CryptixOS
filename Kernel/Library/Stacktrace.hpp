/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Prism/Types.hpp"

#include <compare>

namespace Stacktrace
{
    struct Symbol
    {
        char*                          name;
        uintptr_t                      address;

        constexpr std::strong_ordering operator<=>(const Symbol& rhs) const
        {
            return address <=> rhs.address;
        }
    };

    bool Initialize();
    void Print(usize maxFrames = 16);
}; // namespace Stacktrace
using Stacktrace::Symbol;
