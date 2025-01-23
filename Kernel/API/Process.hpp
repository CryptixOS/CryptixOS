/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

namespace Syscall::Process
{
    ErrorOr<i32>   SysSigProcMask(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGetPid(Syscall::Arguments& args);

    ErrorOr<pid_t> SysFork(Syscall::Arguments& args);
    ErrorOr<i32>   SysExecve(Syscall::Arguments& args);
    ErrorOr<i32>   SysExit(Syscall::Arguments& args);
    ErrorOr<i32>   SysWait4(Syscall::Arguments& args);
    ErrorOr<i32>   SysKill(Syscall::Arguments& args);

    ErrorOr<uid_t> SysGetUid(Syscall::Arguments& args);
    ErrorOr<gid_t> SysGetGid(Syscall::Arguments& args);
    ErrorOr<uid_t> SysGet_eUid(Syscall::Arguments& args);
    ErrorOr<gid_t> SysGet_eGid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysSet_pGid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGet_pPid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGetPgrp(Syscall::Arguments& args);
    ErrorOr<pid_t> SysSetSid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGet_pGid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGetSid(Syscall::Arguments& args);

    ErrorOr<i32>   SysNanoSleep(Syscall::Arguments& args);
}; // namespace Syscall::Process
