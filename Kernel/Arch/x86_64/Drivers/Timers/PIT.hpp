/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

inline static constexpr u64 PIT_BASE_FREQUENCY = 1193182;

namespace PIT
{
    void Sleep(u64 ms);

    void Initialize();

    u8   GetInterruptVector();
    u64  GetCurrentCount();
    u64  GetMilliseconds();

    void SetFrequency(usize frequency);
    void SetReloadValue(u16 reloadValue);

}; // namespace PIT
