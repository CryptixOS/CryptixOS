/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Time.hpp>

#include <Arch/x86_64/Drivers/Time/RTC.hpp>

#include <Scheduler/Process.hpp>
#include <Time/Time.hpp>

namespace API::Time
{
    ErrorOr<isize> GetTimeOfDay(struct timeval* tv, struct timezone* tz)
    {
        Process* current = Process::GetCurrent();

        // TODO(v1tr10l7): Provide better abstraction over time management
        if (tv)
        {
            if (!current->ValidateWrite(tv)) return Error(EFAULT);

            time_t now = 0;
#if CTOS_ARCH_X86_64
            now = RTC::CurrentTime();
#else
            LogWarn("API: SysGetTimeOfDay is not implemented on this platform");
            return Error(ENOSYS);
#endif

            tv->tv_sec  = now;
            tv->tv_usec = 0;
        }
        if (tz)
        {
            if (!current->ValidateWrite(tz)) return Error(EFAULT);

            // TODO(v1tr10l7): SysGetTimeOfDay -> Get time zone
            tz->tz_dsttime     = 0;
            tz->tz_minuteswest = 0;
        }

        return 0;
    }
    ErrorOr<isize> SetTimeOfDay(const struct timeval*  tv,
                                const struct timezone* tz)
    {
        // TODO(v1tr10l7): Check if user has sufficient permissions to set time
        // of day
        Process* current = Process::GetCurrent();
        if (tv)
        {
            if (!current->ValidateRead(tv)) return Error(EFAULT);

            time_t now = 0;
#ifdef CTOS_TARGET_X86_64
            now = RTC::CurrentTime();
#endif
            if (tv->tv_sec < 0 || tv->tv_sec < now || tv->tv_usec < 0
                || tv->tv_usec < now / 1000)
                return Error(EINVAL);

            // TODO(vt1r10l7): Set the time of day
        }
        if (tz)
        {
            if (!current->ValidateRead(tz)) return Error(EFAULT);

            // TODO(vt1r10l7): Set the timezone
        }

        return Error(ENOSYS);
    }
    ErrorOr<isize> ClockGetTime(clockid_t clockID, timespec* res)
    {
        timespec ts;

        switch (clockID)
        {
            case CLOCK_REALTIME:
            case CLOCK_REALTIME_COARSE: ts = ::Time::GetReal(); break;
            case CLOCK_MONOTONIC:
            case CLOCK_MONOTONIC_RAW:
            case CLOCK_MONOTONIC_COARSE:
            case CLOCK_BOOTTIME: ts = ::Time::GetMonotonic(); break;
            case CLOCK_PROCESS_CPUTIME_ID:
            case CLOCK_THREAD_CPUTIME_ID:
                ts = {.tv_sec = 0, .tv_nsec = 0};
                break;

            default: return Error(ENOSYS);
        }

        auto current = Process::GetCurrent();
        if (!current->ValidateWrite(res)) return Error(EFAULT);

        CPU::AsUser([res, &ts]() { *res = ts; });
        return 0;
    }
}; // namespace API::Time
