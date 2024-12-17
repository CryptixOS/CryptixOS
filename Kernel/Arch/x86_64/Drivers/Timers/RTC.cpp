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
    inline static bool IsUpdateInProgress()
    {
        return CMOS::Read(CMOS::Register::eRtcStatusA) & 0x80;
    }
    inline static u8 ReadRegister(CMOS::Register reg)
    {
        while (IsUpdateInProgress()) Arch::Pause();
        u8 value = CMOS::Read(reg);

        return !(CMOS::Read(CMOS::Register::eRtcStatusB) & 0x04)
                 ? BcdToBin(value)
                 : value;
    }

    u8 GetCentury()
    {
        // TODO(V1tri0l1744): Read century from FADT
        return ReadRegister(CMOS::Register::eRtcCentury);
    }
    u8   GetYear() { return ReadRegister(CMOS::Register::eRtcYear); }
    u8   GetMonth() { return ReadRegister(CMOS::Register::eRtcMonth); }
    u8   GetDay() { return ReadRegister(CMOS::Register::eRtcDayOfMonth); }
    u8   GetHour() { return ReadRegister(CMOS::Register::eRtcHours); }
    u8   GetMinute() { return ReadRegister(CMOS::Register::eRtcMinutes); }
    u8   GetSecond() { return ReadRegister(CMOS::Register::eRtcSeconds); }

    u8   GetTime() { return GetHour() * 3600 + GetMinute() * 60 + GetSecond(); }

    void Sleep(u64 seconds)
    {
        u64 lastSecond = GetTime();
        while (lastSecond == GetTime()) Arch::Pause();

        lastSecond = GetTime() + seconds;
        while (lastSecond != GetTime()) Arch::Pause();
    }
}; // namespace RTC
