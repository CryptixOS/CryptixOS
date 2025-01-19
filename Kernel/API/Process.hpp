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
    std::expected<pid_t, std::errno_t> SysGetPid(Syscall::Arguments& args);

    std::expected<pid_t, std::errno_t> SysFork(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysExecve(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysExit(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysWait4(Syscall::Arguments& args);

    std::expected<uid_t, std::errno_t> SysGetUid(Syscall::Arguments& args);
    std::expected<gid_t, std::errno_t> SysGetGid(Syscall::Arguments& args);
    std::expected<uid_t, std::errno_t> SysGet_eUid(Syscall::Arguments& args);
    std::expected<gid_t, std::errno_t> SysGet_eGid(Syscall::Arguments& args);
    std::expected<pid_t, std::errno_t> SysGet_pPid(Syscall::Arguments& args);
}; // namespace Syscall::Process
