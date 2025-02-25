/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Prism/Types.hpp"

namespace PIT
{
    constexpr usize BASE_FREQUENCY  = 1193182ull;

    constexpr usize SEND_WORD       = 0x30;

    constexpr usize CHANNEL0_DATA   = 0x40;
    constexpr usize CHANNEL1_DATA   = 0x41;
    constexpr usize CHANNEL2_DATA   = 0x42;
    constexpr usize COMMAND         = 0x43;

    constexpr usize SELECT_CHANNEL0 = 0x00;
    constexpr usize SELECT_CHANNEL1 = 0x40;
    constexpr usize SELECT_CHANNEL2 = 0x80;

    namespace Mode
    {
        constexpr usize COUNTDOWN   = 0x00;
        constexpr usize ONESHOT     = 0x02;
        constexpr usize RATE        = 0x04;
        constexpr usize SQUARE_WAVE = 0x06;
    }; // namespace Mode

    void Initialize();

    void Start(usize mode, usize ms);
    void Stop();

    u8   GetInterruptVector();

    u64  GetCurrentCount();
    u64  GetMilliseconds();

    void SetFrequency(usize frequency);
    void SetReloadValue(u16 reloadValue);

    void Sleep(u64 ms);
}; // namespace PIT
