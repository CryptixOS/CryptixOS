/*
 * Created by v1tr10l7 on 19.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/sys/mman.h>
#include <API/Posix/sys/time.h>
#include <API/Time.hpp>

#include <Scheduler/Process.hpp>

namespace Syscall::Time
{
    std::expected<i32, errno_t> SysGetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        Process*  current = Process::GetCurrent();

        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return errno_t(EFAULT);

            // TODO(v1tr10l7): get time of day
            tv->tv_sec  = 0;
            tv->tv_usec = 0;
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return errno_t(EFAULT);

            tz->tz_dsttime     = 0;
            tz->tz_minuteswest = 0;
        }

        return errno_t(ENOSYS);
    }
    std::expected<i32, errno_t> SysSetTimeOfDay(Arguments& args)
    {
        timeval*  tv      = reinterpret_cast<timeval*>(args.Args[0]);
        timezone* tz      = reinterpret_cast<timezone*>(args.Args[1]);

        // TODO(v1tr10l7): Check if user has sufficient permissions to set time
        // of day
        Process*  current = Process::GetCurrent();
        if (tv)
        {
            if (!current->ValidateAddress(tv, PROT_READ | PROT_WRITE))
                return errno_t(EFAULT);

            // TODO(vt1r10l7): Set the time of day
        }
        if (tz)
        {
            if (!current->ValidateAddress(tz, PROT_READ | PROT_WRITE))
                return errno_t(EFAULT);

            // TODO(vt1r10l7): Set the timezone
        }

        return errno_t(ENOSYS);
    }
} // namespace Syscall::Time
