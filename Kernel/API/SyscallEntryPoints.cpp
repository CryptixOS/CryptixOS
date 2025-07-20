
/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/Posix/fcntl.h>
#include <API/SyscallEntryPoints.hpp>
#include <API/System.hpp>
#include <API/Time.hpp>
#include <API/VFS.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>

#include <Time/Time.hpp>

namespace Syscall
{
    namespace API = API::VFS;
    using namespace ::API;

    ErrorOr<isize> SysPipe(Arguments& args)
    {
        i32*     pipeFds = args.Get<i32*>(0);

        Process* current = Process::GetCurrent();
        return current->OpenPipe(pipeFds);
    }
    ErrorOr<isize> SysNanoSleep(Arguments& args)
    {
        LogDebug("SysNanoSleep");
        const timespec* duration = args.Get<const timespec*>(0);
        timespec*       rem      = args.Get<timespec*>(1);

        Process*        current  = Process::GetCurrent();
        if (!current->ValidateRead(duration, sizeof(timespec))
            || (rem && !current->ValidateRead(rem, sizeof(timespec))))
            return Error(EFAULT);
        if (duration->tv_sec < 0 || duration->tv_nsec < 0
            || (rem && (rem->tv_sec < 0 || rem->tv_nsec < 0)))
            return Error(EINVAL);

        auto errorOr = ::Time::Sleep(duration, rem);
        if (!errorOr) return errorOr.error();

        return 0;
    }
    ErrorOr<isize> SysTruncate(Arguments& args)
    {
        const char* path   = args.Get<const char*>(0);
        off_t       length = args.Get<off_t>(1);

        return API::Truncate(path, length);
    }
    ErrorOr<isize> SysFTruncate(Arguments& args)
    {
        i32   fdNum  = args.Get<i32>(0);
        off_t length = args.Get<off_t>(1);

        return API::FTruncate(fdNum, length);
    }
    ErrorOr<isize> SysReadLink(Arguments& args)
    {
        const char* path = args.Get<const char*>(0);
        char*       out  = args.Get<char*>(1);
        usize       size = args.Get<usize>(2);

        return API::ReadLink(path, out, size);
    }
    ErrorOr<isize> SysClockGetTime(Arguments& args)
    {
        clockid_t id  = args.Get<clockid_t>(0);
        timespec* res = args.Get<timespec*>(1);

        return ::API::Time::SysClockGetTime(id, res);
    }
}; // namespace Syscall
