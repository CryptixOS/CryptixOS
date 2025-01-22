/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>
#include <Time/Time.hpp>

namespace Time
{
    // TODO(v1tr10l7): implement real and monotonic clocks
    usize GetEpoch() { return Arch::GetEpoch(); }
    usize GetReal() { return GetEpoch(); }
    usize GetMonotonic() { return GetEpoch(); }
}; // namespace Time
