/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/mman.h>
#include <API/Posix/sys/time.h>
#include <API/Time.hpp>

#include <Arch/x86_64/Drivers/Timers/RTC.hpp>

#include <Scheduler/Process.hpp>

namespace Syscall::Time
{
    std::expected<i32, std::errno_t> SysGetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        Process*  current = Process::GetCurrent();

        // TODO(v1tr10l7): Provide better abstraction over time management
        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return std::errno_t(EFAULT);

            time_t now  = RTC::CurrentTime();
            tv->tv_sec  = now;
            tv->tv_usec = 0;
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return std::errno_t(EFAULT);

            tz->tz_dsttime     = 0;
            tz->tz_minuteswest = 0;
        }

        return std::errno_t(ENOSYS);
    }
    std::expected<i32, std::errno_t> SysSetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        // TODO(v1tr10l7): Check if user has sufficient permissions to set time
        // of day
        Process*  current = Process::GetCurrent();
        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return std::errno_t(EFAULT);

            time_t now = RTC::CurrentTime();
            if (tv->tv_sec < 0 || tv->tv_sec < now || tv->tv_usec < 0
                || tv->tv_usec < now / 1000)
                return std::errno_t(EINVAL);

            // TODO(vt1r10l7): Set the time of day
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return std::errno_t(EFAULT);

            // TODO(vt1r10l7): Set the timezone
        }

        return std::errno_t(ENOSYS);
    }
} // namespace Syscall::Time
