/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/time.h>

#include <Prism/Types.hpp>

namespace RTC
{
    constexpr u8 BcdToBin(u8 value) { return (value >> 4) * 10 + (value & 15); }

    u8           GetCentury();
    u8           GetYear();
    u8           GetMonth();
    u8           GetDay();
    u8           GetHour();
    u8           GetMinute();
    u8           GetSecond();

    time_t       CurrentTime();
    u8           GetTime();

    void         Sleep(u64 seconds);
}; // namespace RTC
