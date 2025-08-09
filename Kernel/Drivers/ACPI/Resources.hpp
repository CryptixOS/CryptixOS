/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

namespace ACPI
{
    struct IrqResource
    {
        u8         Triggering;
        u8         Polarity;
        u8         Sharing;
        u8         WakeCapability;
        Vector<u8> IRQs;
    };
    struct IoResource
    {
        u16 Least;
        u16 Highest;
        u8  Alignment;
        u8  Length;
    };
}; // namespace ACPI
