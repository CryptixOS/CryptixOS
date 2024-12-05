/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Types.hpp>

namespace CMOS
{
    // http://www.bioscentral.com/misc/cmosmap.htm
    constexpr byte RTC_SECONDS         = 0x00;
    constexpr byte RTC_MINUTES         = 0x02;
    constexpr byte RTC_HOURS           = 0x04;
    constexpr byte RTC_WEEK_DAY        = 0x06;
    constexpr byte RTC_MONTH_DAY       = 0x07;
    constexpr byte RTC_MONTH           = 0x08;
    constexpr byte RTC_YEAR            = 0x09;
    constexpr byte STATUS_REGISTER_A   = 0x0a;
    constexpr byte STATUS_REGISTER_B   = 0x0b;
    constexpr byte STATUS_REGISTER_C   = 0x0c;
    constexpr byte STATUS_REGISTER_D   = 0x0d;
    constexpr byte DIAGNOSTIC_STATUS   = 0x0e;
    constexpr byte SHUTDOWN_STATUS     = 0x0f;
    constexpr byte FLOPPY_TYPES        = 0x10;
    constexpr byte SYS_CONFIG_SETTINGS = 0x11;
    constexpr byte HARD_DISK_TYPES     = 0x12;
    constexpr byte LOW_MEMORY_L        = 0x15;
    constexpr byte LOW_MEMORY_H        = 0x16;
    constexpr byte MIDDLE_MEMORY_L     = 0x17;
    constexpr byte MIDDLE_MEMORY_H     = 0x18;
    constexpr byte HIGH_MEMORY_L       = 0x30;
    constexpr byte HIGH_MEMORY_H       = 0x31;
    constexpr byte RTC_CENTURY         = 0x32;

    void           Write(byte reg, byte value);
    byte           Read(byte reg);
} // namespace CMOS
