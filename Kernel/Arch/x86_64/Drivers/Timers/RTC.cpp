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
    enum class Register
    {
        eCentury = CMOS::RTC_CENTURY,
        eYear    = CMOS::RTC_YEAR,
        eMonth   = CMOS::RTC_MONTH,
        eDay     = CMOS::RTC_MONTH_DAY,
        eHour    = CMOS::RTC_HOURS,
        eMinute  = CMOS::RTC_MINUTES,
        eSecond  = CMOS::RTC_SECONDS,
    };

    inline static bool IsUpdateInProgress()
    {
        return CMOS::Read(CMOS::STATUS_REGISTER_A) & 0x80;
    }
    inline static u8 ReadRegister(Register reg)
    {
        while (IsUpdateInProgress()) Arch::Pause();
        u8 value = CMOS::Read(std::to_underlying(reg));

        return !(CMOS::Read(CMOS::STATUS_REGISTER_B) & 0x04) ? BcdToBin(value)
                                                             : value;
    }

    u8 GetCentury()
    {
        // TODO(V1tri0l1744): Read century from FADT
        return ReadRegister(RTC::Register::eCentury);
    }
    u8   GetYear() { return ReadRegister(RTC::Register::eYear); }
    u8   GetMonth() { return ReadRegister(RTC::Register::eMonth); }
    u8   GetDay() { return ReadRegister(RTC::Register::eDay); }
    u8   GetHour() { return ReadRegister(RTC::Register::eHour); }
    u8   GetMinute() { return ReadRegister(RTC::Register::eMinute); }
    u8   GetSecond() { return ReadRegister(RTC::Register::eSecond); }

    u8   GetTime() { return GetHour() * 3600 + GetMinute() * 60 + GetSecond(); }

    void Sleep(u64 seconds)
    {
        u64 lastSecond = GetTime();
        while (lastSecond == GetTime()) Arch::Pause();

        lastSecond = GetTime() + seconds;
        while (lastSecond != GetTime()) Arch::Pause();
    }
}; // namespace RTC
