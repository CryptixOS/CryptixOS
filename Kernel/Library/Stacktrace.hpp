/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/String.hpp>

#include <compare>

namespace Stacktrace
{
    struct StackFrame
    {
        StackFrame* Base;
        Pointer     InstructionPointer;
    };
    struct Symbol
    {
        String                         Name;
        uintptr_t                      Address;

        constexpr std::strong_ordering operator<=>(const Symbol& rhs) const
        {
            return Address <=> rhs.Address;
        }
    };

    bool Initialize();
    void Print(usize maxFrames = 16);
}; // namespace Stacktrace
using KernelSymbol = Stacktrace::Symbol;
