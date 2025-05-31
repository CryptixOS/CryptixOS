/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace Arch
{
    constexpr inline bool InUserRange(Pointer virt, usize bytes)
    {
        return !virt.IsHigherHalf();
    }

    ErrorOr<void> CopyToUser(Pointer destination, Pointer source, usize count);
    ErrorOr<void> CopyFromUser(Pointer destination, Pointer source,
                               usize count);
    ErrorOr<void> FillUser(Pointer buffer, isize value, usize count);
}; // namespace Arch
