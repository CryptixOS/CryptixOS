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
    using namespace ::Time;
    ErrorOr<isize> NanoSleep(const struct timespec* duration, timespec* rem)
    {
        auto current = Process::Current();
        if ((duration && !current->ValidateRead(duration, sizeof(timespec)))
            || (rem && !current->ValidateRead(rem, sizeof(timespec))))
            return Error(EFAULT);

        usize ns = 0;
        {
            timespec sleepDuration = {};
            if (duration) sleepDuration = CopyFromUser(*duration);
            if (sleepDuration.tv_sec < 0 || sleepDuration.tv_nsec < 0)
                return Error(EINVAL);

            ns = sleepDuration.tv_nsec ?: sleepDuration.tv_sec * 1'000'000'000;
        }
        auto status = ::Time::NanoSleep(ns);
        if (!status) return Error(status.Error());

        timespec reminder{};
        if (rem) CopyToUser(rem, reminder);
        return 0;
    }
    ErrorOr<isize> GetITimer(isize which, struct itimerval* currentValue)
    {
        return Error(ENOSYS);
    }

    ErrorOr<isize> SetITimer(isize which, const struct itimerval* value,
                             struct itimerval* oldValue)
    {
        auto      process  = Process::Current();
        itimerval newValue = {};

        if (!value) return Error(EINVAL);
        if (!process->ValidateRead(value, sizeof(itimerval)))
            return Error(EFAULT);
        // FIXME(v1tr10l7): Validate <which>
        newValue = CopyFromUser(*value);

        LogDebug(
            "SetITimer: value: {{ .it_interval: {{ .tv_usec: {}, .tv_sec: {} "
            "}}, .it_value: {{ .tv_usec: {}, .tv_sec: {} }} }}",
            newValue.it_interval.tv_usec, newValue.it_interval.tv_sec,
            newValue.it_value.tv_usec, newValue.it_value.tv_sec);
        if (!newValue.it_value || !newValue.it_interval) return Error(EINVAL);
        if (which != ITIMER_REAL && which != ITIMER_VIRTUAL
            && which != ITIMER_PROF)
            return Error(EINVAL);

        auto      timer = process->Timer(which);
        itimerval previousState;
        previousState.it_interval.tv_usec = timer->When.Microseconds();
        previousState.it_interval.tv_sec  = 0;
        previousState.it_value.tv_usec    = timer->ReloadValue.Microseconds();
        previousState.it_value.tv_sec     = 0;

        auto  expirationPeriod            = newValue.it_interval;
        usize ns                          = expirationPeriod.tv_usec;
        if (ns) ns *= 1'000;
        else ns = expirationPeriod.tv_sec * 1'000'000'000;
        usize reloadValue = newValue.it_value.tv_usec;
        if (reloadValue) reloadValue *= 1'000;
        else reloadValue *= 1'000'000'000;

        // TODO(v1tr10l7): generate a signal
        if (ns)
            Time::ArmTimer(timer, ns, []() { LogTrace("Fired"); }, reloadValue);
        else Time::DisarmTimer(timer);

        if (oldValue)
        {
            if (!process->ValidateWrite(oldValue)) return Error(EFAULT);
            CopyToUser(oldValue, previousState);
        }
        return 0;
    }

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
            case CLOCK_REALTIME_COARSE: ts = Time::GetReal(); break;
            case CLOCK_MONOTONIC:
            case CLOCK_MONOTONIC_RAW:
            case CLOCK_MONOTONIC_COARSE:
            case CLOCK_BOOTTIME: ts = Time::GetMonotonic(); break;
            case CLOCK_PROCESS_CPUTIME_ID:
            case CLOCK_THREAD_CPUTIME_ID:
                ts = {.tv_sec = 0, .tv_nsec = 0};
                break;

            default: return Error(ENOSYS);
        }

        auto current = Process::GetCurrent();
        if (!current->ValidateWrite(res)) return Error(EFAULT);

        AsUser([res, &ts]() { *res = ts; });
        return 0;
    }
}; // namespace API::Time
