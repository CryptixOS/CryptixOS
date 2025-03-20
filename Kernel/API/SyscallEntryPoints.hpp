/*
 * Created by v1tr10l7 on 22.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>

namespace Syscall
{
    ErrorOr<isize> SysRead(Arguments& args);
    ErrorOr<isize> SysWrite(Arguments& args);
    ErrorOr<isize> SysOpen(Arguments& args);
    ErrorOr<isize> SysClose(Arguments& args);

    ErrorOr<isize> SysStat(Arguments& args);
    ErrorOr<isize> SysFStat(Arguments& args);
    ErrorOr<isize> SysLStat(Arguments& args);
    ErrorOr<isize> SysLSeek(Arguments& args);

    ErrorOr<isize> SysMMap(Arguments& args);
    ErrorOr<isize> SysIoCtl(Arguments& args);
    ErrorOr<isize> SysAccess(Arguments& args);
    ErrorOr<isize> SysSchedYield(Arguments& args);
    ErrorOr<isize> SysDup(Arguments& args);
    ErrorOr<isize> SysDup2(Arguments& args);
    ErrorOr<isize> SysNanoSleep(Arguments& args);
    ErrorOr<isize> SysGetPid(Arguments& args);
    ErrorOr<isize> SysExit(Arguments& args);
    ErrorOr<isize> SysWait4(Arguments& args);
    ErrorOr<isize> SysGetUid(Arguments& args);
    ErrorOr<isize> SysGetGid(Arguments& args);
    ErrorOr<isize> SysUname(Arguments& args);
    ErrorOr<isize> SysFcntl(Arguments& args);
    ErrorOr<isize> SysTruncate(Arguments& args);
    ErrorOr<isize> SysFTruncate(Arguments& args);
    ErrorOr<isize> SysGetCwd(Arguments& args);
    ErrorOr<isize> SysChDir(Arguments& args);
    ErrorOr<isize> SysFChDir(Arguments& args);
    ErrorOr<isize> SysMkDir(Arguments& args);
    ErrorOr<isize> SysRmDir(Arguments& args);
    ErrorOr<isize> SysCreat(Arguments& args);
    ErrorOr<isize> SysReadLink(Arguments& args);
    ErrorOr<isize> SysUmask(Arguments& args);
    ErrorOr<isize> SysGetTimeOfDay(Arguments& args);
    ErrorOr<isize> SysGet_eUid(Arguments& args);
    ErrorOr<isize> SysGet_eGid(Arguments& args);
    ErrorOr<isize> SysSet_pGid(Arguments& args);
    ErrorOr<isize> SysGet_pPid(Arguments& args);
    ErrorOr<isize> SysGet_pGid(Arguments& args);
    ErrorOr<isize> SysUTime(Arguments& args);
    ErrorOr<isize> SysFork(Arguments& args);
    ErrorOr<isize> SysExecve(Arguments& args);
    // ErrorOr<isize> SysArchPrCtl(Arguments& args);
    ErrorOr<isize> SysSetTimeOfDay(Arguments& args);
    ErrorOr<isize> SysReboot(Arguments& args);
    ErrorOr<isize> SysGetDents64(Arguments& args);
    ErrorOr<isize> SysClockGetTime(Arguments& args);
    ErrorOr<isize> SysOpenAt(Arguments& args);
    ErrorOr<isize> SysMkDirAt(Arguments& args);
    ErrorOr<isize> SysFStatAt(Arguments& args);
    ErrorOr<isize> SysDup3(Arguments& args);

}; // namespace Syscall
