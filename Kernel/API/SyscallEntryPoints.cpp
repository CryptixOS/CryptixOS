
/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/SyscallEntryPoints.hpp>
#include <API/VFS.hpp>

namespace Syscall
{
    namespace API = API::VFS;

    ErrorOr<isize> SysRead(Arguments& args)
    {
        i32   fdNum = args.Get<i32>(0);
        u8*   out   = args.Get<u8*>(1);
        usize bytes = args.Get<usize>(2);

        return API::SysRead(fdNum, out, bytes);
    }
    ErrorOr<isize> SysWrite(Arguments& args)
    {
        i32       fdNum = args.Get<i32>(0);
        const u8* input = args.Get<const u8*>(1);
        usize     bytes = args.Get<usize>(2);

        return API::SysWrite(fdNum, input, bytes);
    }
    ErrorOr<isize> SysOpen(Arguments& args)
    {
        const char* path  = args.Get<const char*>(0);
        i32         flags = args.Get<i32>(1);
        mode_t      mode  = args.Get<mode_t>(2);

        return API::SysOpen(path, flags, mode);
    }
    ErrorOr<isize> SysClose(Arguments& args)
    {
        i32 fdNum = args.Get<i32>(0);

        return API::SysClose(fdNum);
    }

    ErrorOr<isize> SysStat(Arguments& args);
    ErrorOr<isize> SysFStat(Arguments& args);
    ErrorOr<isize> SysLStat(Arguments& args);
    ErrorOr<isize> SysLSeek(Arguments& args);

    ErrorOr<isize> SysMMap(Arguments& args);
    ErrorOr<isize> SysIoCtl(Arguments& args);
    ErrorOr<isize> SysAccess(Arguments& args);
    ErrorOr<isize> SysDup(Arguments& args);
    ErrorOr<isize> SysDup2(Arguments& args);
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

        return API::SysTruncate(path, length);
    }
    ErrorOr<isize> SysFTruncate(Arguments& args)
    {
        i32   fdNum  = args.Get<i32>(0);
        off_t length = args.Get<off_t>(1);

        return API::SysFTruncate(fdNum, length);
    }
    ErrorOr<isize> SysGetCwd(Arguments& args);
    ErrorOr<isize> SysChDir(Arguments& args);
    ErrorOr<isize> SysFChDir(Arguments& args);
    ErrorOr<isize> SysMkDir(Arguments& args);
    ErrorOr<isize> SysRmDir(Arguments& args)
    {
        const char* path = args.Get<const char*>(0);

        return API::SysRmDir(path);
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

        return API::SysUTime(path, times);
    }
    ErrorOr<isize> SysFork(Arguments& args);
    ErrorOr<isize> SysExecve(Arguments& args);
    ErrorOr<isize> SysArchPrCtl(Arguments& args);
    ErrorOr<isize> SysSetTimeOfDay(Arguments& args);
    ErrorOr<isize> SysGetDents64(Arguments& args);
    ErrorOr<isize> SysOpenAt(Arguments& args);
    ErrorOr<isize> SysMkDirAt(Arguments& args);
    ErrorOr<isize> SysFStatAt(Arguments& args);
    ErrorOr<isize> SysDup3(Arguments& args);

}; // namespace Syscall
