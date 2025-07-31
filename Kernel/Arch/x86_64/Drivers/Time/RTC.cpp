/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/Time/RTC.hpp>

#include <Arch/Arch.hpp>
#include <Arch/x86_64/CMOS.hpp>

#include <Firmware/ACPI/ACPI.hpp>
#include <Prism/Utility/Time.hpp>

namespace RTC
{
    namespace
    {
        std::optional<bool> s_FormatInBcd = std::nullopt;

        inline bool         IsUpdateInProgress()
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
        while (IsUpdateInProgress())
            ;

        if (!s_FormatInBcd.has_value())
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
