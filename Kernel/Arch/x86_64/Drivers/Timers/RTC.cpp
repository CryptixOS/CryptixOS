/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/Timers/RTC.hpp>

#include <Arch/Arch.hpp>
#include <Arch/x86_64/CMOS.hpp>

#include <utility>

namespace RTC
{
    namespace
    {
        inline bool IsUpdateInProgress()
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

        inline bool IsLeapYear(u32 year)
        {
            return ((year % 4 == 0)
                    && ((year % 100 != 0) || (year % 400) == 0));
        }
        u32 DaysInMonth(u32 month, u32 year)
        {
            switch (month)
            {
                case 11: return 30;
                case 10: return 31;
                case 9: return 30;
                case 8: return 31;
                case 7: return 31;
                case 6: return 30;
                case 5: return 31;
                case 4: return 30;
                case 3: return 31;
                case 2:
                    if (IsLeapYear(year)) return 29;
                    return 28;
                case 1: return 31;
                default: return 0;
            }
        }

        unsigned DaysSinceEpoch(u32 year)
        {
            u32 days = 0;
            while (year > 1969)
            {
                days += 365;
                if (IsLeapYear(year)) ++days;
                --year;
            }
            return days;
        }
    } // namespace

    u8 GetCentury()
    {
        // TODO(V1tri0l1744): Read century from FADT
        return ReadRegister(CMOS::Register::eRtcCentury);
    }
    u8     GetYear() { return ReadRegister(CMOS::Register::eRtcYear); }
    u8     GetMonth() { return ReadRegister(CMOS::Register::eRtcMonth); }
    u8     GetDay() { return ReadRegister(CMOS::Register::eRtcDayOfMonth); }
    u8     GetHour() { return ReadRegister(CMOS::Register::eRtcHours); }
    u8     GetMinute() { return ReadRegister(CMOS::Register::eRtcMinutes); }
    u8     GetSecond() { return ReadRegister(CMOS::Register::eRtcSeconds); }

    time_t CurrentTime()
    {
        while (IsUpdateInProgress());

        return DaysSinceEpoch(GetYear() - 1) * 86400
             + DaysInMonth(GetMonth() - 1, GetYear()) * 86400 + GetDay() * 86400
             + GetHour() * 3600 + GetMinute() * 60 + GetSecond();
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
