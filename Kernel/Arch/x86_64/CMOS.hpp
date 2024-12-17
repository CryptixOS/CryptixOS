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
    // https://www.singlix.com/trdos/archive/pdf_archive/CMOS%20RAM%20Map.pdf
    enum class Register
    {
        // RTC registers
        eRtcSeconds           = 0x00,
        eRtcSecondsAlarm      = 0x01,
        eRtcMinutes           = 0x02,
        eRtcMinutesAlarm      = 0x03,
        eRtcHours             = 0x04,
        eRtcHoursAlarm        = 0x05,
        eRtcDayOfWeek         = 0x06,
        eRtcDayOfMonth        = 0x07,
        eRtcMonth             = 0x08,
        eRtcYear              = 0x09,
        eRtcStatusA           = 0x0a,
        eRtcStatusB           = 0x0b,
        eRtcStatusC           = 0x0c,
        eRtcStatusD           = 0x0d,
        eDiagnosticStatus     = 0x0e,
        eShutdownStatus       = 0x0f,

        // ISA configuration registers
        eFloppyDriveType      = 0x10,

        eEquipmentByte        = 0x14,

        // Base Memory
        eBaseMemoryLow        = 0x15,
        eBaseMemoryHigh       = 0x16,

        // Extended BIOS memory map middle
        eExtendedMemoryLow    = 0x17,
        eExtendedMemoryHigh   = 0x18,

        // Extended BIOS memory map in KB
        eExtendedMemoryKbLow  = 0x30,
        eExtendedMemoryKbHigh = 0x31,
        eConfigurationByte    = 0x2d,
        eRtcCentury           = 0x32,
    };

    void Write(Register reg, byte value);
    byte Read(Register reg);
} // namespace CMOS
