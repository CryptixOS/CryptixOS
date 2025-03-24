
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

    ErrorOr<isize> SysRead(Arguments& args)
    {
        i32   fdNum = args.Get<i32>(0);
        u8*   out   = args.Get<u8*>(1);
        usize bytes = args.Get<usize>(2);

        return API::Read(fdNum, out, bytes);
    }
    ErrorOr<isize> SysWrite(Arguments& args)
    {
        i32       fdNum = args.Get<i32>(0);
        const u8* input = args.Get<const u8*>(1);
        usize     bytes = args.Get<usize>(2);

        return API::Write(fdNum, input, bytes);
    }
    ErrorOr<isize> SysOpen(Arguments& args)
    {
        const char* path  = args.Get<const char*>(0);
        i32         flags = args.Get<i32>(1);
        mode_t      mode  = args.Get<mode_t>(2);

        return API::Open(path, flags, mode);
    }
    ErrorOr<isize> SysClose(Arguments& args)
    {
        i32 fdNum = args.Get<i32>(0);

        return API::Close(fdNum);
    }

    ErrorOr<isize> SysStat(Arguments& args);
    ErrorOr<isize> SysFStat(Arguments& args);
    ErrorOr<isize> SysLStat(Arguments& args);
    ErrorOr<isize> SysLSeek(Arguments& args);

    ErrorOr<isize> SysMMap(Arguments& args)
    {
        uintptr_t addr   = args.Get<uintptr_t>(0);
        usize     len    = args.Get<usize>(1);
        i32       prot   = args.Get<uintptr_t>(2);
        i32       flags  = args.Get<uintptr_t>(3);
        i32       fdNum  = args.Get<uintptr_t>(4);
        off_t     offset = args.Get<uintptr_t>(5);

        return ::API::MM::SysMMap(addr, len, prot, flags, fdNum, offset);
    }
    ErrorOr<isize> SysIoCtl(Arguments& args);
    ErrorOr<isize> SysAccess(Arguments& args);
    ErrorOr<isize> SysPipe(Arguments& args)
    {
        i32*     pipeFds = args.Get<i32*>(0);

        Process* current = Process::GetCurrent();
        return current->OpenPipe(pipeFds);
    }
    ErrorOr<isize> SysSchedYield(Arguments& args)
    {
        Scheduler::Yield();
        return 0;
    }
    ErrorOr<isize> SysDup(Arguments& args);
    ErrorOr<isize> SysDup2(Arguments& args);
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
    ErrorOr<isize> SysGetPid(Arguments& args);
    ErrorOr<isize> SysExit(Arguments& args);
    ErrorOr<isize> SysWait4(Arguments& args);
    ErrorOr<isize> SysGetUid(Arguments& args);
    ErrorOr<isize> SysGetGid(Arguments& args);
    ErrorOr<isize> SysUname(Arguments& args);
    ErrorOr<isize> SysFcntl(Arguments& args);
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
    ErrorOr<isize> SysGetCwd(Arguments& args)
    {
        char* buffer = args.Get<char*>(0);
        usize size   = args.Get<usize>(1);

        return API::GetCwd(buffer, size);
    }
    ErrorOr<isize> SysChDir(Arguments& args);
    ErrorOr<isize> SysFChDir(Arguments& args);
    ErrorOr<isize> SysMkDir(Arguments& args);
    ErrorOr<isize> SysRmDir(Arguments& args)
    {
        const char* path = args.Get<const char*>(0);

        return API::RmDir(path);
    }
    ErrorOr<isize> SysCreat(Arguments& args)
    {
        const char* path = args.Get<const char*>(0);
        mode_t      mode = args.Get<mode_t>(1);

        return API::Open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
    }
    ErrorOr<isize> SysReadLink(Arguments& args)
    {
        const char* path = args.Get<const char*>(0);
        char*       out  = args.Get<char*>(1);
        usize       size = args.Get<usize>(2);

        return API::ReadLink(path, out, size);
    }
    ErrorOr<isize> SysChMod(Arguments& args)
    {
        [[maybe_unused]] const char* path = args.Get<const char*>(0);
        [[maybe_unused]] mode_t      mode = args.Get<mode_t>(1);

        return Error(ENOSYS);
        // return ::API::VFS::SysChModAt(AT_FDCWD, path, mode);
    }
    ErrorOr<isize> SysUmask(Arguments& args)
    {
        mode_t   mask    = args.Get<mode_t>(0);
        Process* current = Process::GetCurrent();

        return current->Umask(mask);
    }
    ErrorOr<isize> SysGetTimeOfDay(Arguments& args);
    ErrorOr<isize> SysGet_eUid(Arguments& args);
    ErrorOr<isize> SysGet_eGid(Arguments& args);
    ErrorOr<isize> SysSet_pGid(Arguments& args);
    ErrorOr<isize> SysGet_pPid(Arguments& args);
    ErrorOr<isize> SysGet_pGid(Arguments& args);
    ErrorOr<isize> SysUTime(Arguments& args)
    {
        const char*    path  = args.Get<const char*>(0);
        const utimbuf* times = args.Get<const utimbuf*>(1);

        return API::UTime(path, times);
    }
    ErrorOr<isize> SysFork(Arguments& args);
    ErrorOr<isize> SysExecve(Arguments& args);
    ErrorOr<isize> SysArchPrCtl(Arguments& args);
    ErrorOr<isize> SysSetTimeOfDay(Arguments& args);
    ErrorOr<isize> SysReboot(Arguments& args)
    {
        using System::RebootCommand;
        RebootCommand cmd = args.Get<RebootCommand>(0);

        return System::SysReboot(cmd);
    }
    ErrorOr<isize> SysGetDents64(Arguments& args);
    ErrorOr<isize> SysClockGetTime(Arguments& args)
    {
        clockid_t id  = args.Get<clockid_t>(0);
        timespec* res = args.Get<timespec*>(1);

        return ::API::Time::SysClockGetTime(id, res);
    }
    ErrorOr<isize> SysOpenAt(Arguments& args);
    ErrorOr<isize> SysMkDirAt(Arguments& args);
    ErrorOr<isize> SysFStatAt(Arguments& args);
    ErrorOr<isize> SysDup3(Arguments& args);

}; // namespace Syscall
