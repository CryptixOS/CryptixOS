/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/time.h>
#include <Arch/Arch.hpp>
#include <Compiler.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Time.hpp>

module Cryptix:Impl.Arch.x86_64.Drivers.Time.RTC;

import :Arch.x86_64.Drivers.Time.RTC;
import :Arch.Arch;
import :Arch.x86_64.CMOS;
import :Firmware.ACPI.ACPI;

namespace RTC
{
    namespace
    {
        Optional<bool> s_FormatInBcd = NullOpt;

        inline bool    IsUpdateInProgress()
        {
            return CMOS::Read(CMOS::Register::eRtcStatusA) & 0x80;
        }
        inline u8 ReadRegister(CMOS::Register reg)
        {
            while (IsUpdateInProgress()) Arch::Pause();
            u8 value = CMOS::Read(reg);

            return !(CMOS::Read(CMOS::Register::eRtcStatusB) & 0x04)
                     ? BcdToBin(value)
                     : value;
        }
    } // namespace

    u8 GetCentury()
    {
        bool centuryAvailable = ACPI::GetCentury();
        if (!centuryAvailable) return 20;

        auto century = ReadRegister(CMOS::Register::eRtcCentury);
        return century;
    }
    u8 GetYear()
    {
        auto year = ReadRegister(CMOS::Register::eRtcYear);

        return year;
    }
    u8 GetMonth()
    {
        auto month = ReadRegister(CMOS::Register::eRtcMonth);

        return month;
    }
    u8 GetDay()
    {
        auto day = ReadRegister(CMOS::Register::eRtcDayOfMonth);

        return day;
    }
    u8 GetHour()
    {
        auto hour = ReadRegister(CMOS::Register::eRtcHours);

        return hour;
    }
    u8 GetMinute()
    {
        auto minute = ReadRegister(CMOS::Register::eRtcMinutes);

        return minute;
    }
    u8 GetSecond()
    {
        auto second = ReadRegister(CMOS::Register::eRtcSeconds);

        return second;
    }

    time_t CurrentTime()
    {
        while (IsUpdateInProgress());

        if (!s_FormatInBcd.HasValue())
            s_FormatInBcd = !(ReadRegister(CMOS::Register::eRtcStatusB) & 0x04);
        DateTime date;
        date.Year   = GetCentury() * 100 + GetYear();
        date.Month  = GetMonth();
        date.Day    = GetDay();
        date.Hour   = GetHour();
        date.Minute = GetMinute();
        date.Second = GetSecond();

        return date.ToEpoch();
    }
    u8   GetTime() { return GetHour() * 3600 + GetMinute() * 60 + GetSecond(); }

    void Sleep(u64 seconds)
    {
        u64 lastSecond = GetTime();
        while (lastSecond == GetTime()) Arch::Pause();

        lastSecond = GetTime() + seconds;
        while (lastSecond != GetTime()) Arch::Pause();
    }
}; // namespace RTC
