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
    pid_t SysGetPid(Syscall::Arguments& args);

    pid_t SysFork(Syscall::Arguments& args);
    i32   SysExecve(Syscall::Arguments& args);
    i32   SysExit(Syscall::Arguments& args);
    i32   SysWait4(Syscall::Arguments& args);

    uid_t SysGetUid(Syscall::Arguments& args);
    gid_t SysGetGid(Syscall::Arguments& args);
    uid_t SysGet_eUid(Syscall::Arguments& args);
    gid_t SysGet_eGid(Syscall::Arguments& args);
    pid_t SysGet_pPid(Syscall::Arguments& args);
}; // namespace Syscall::Process
