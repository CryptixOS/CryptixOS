/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/Syscall.hpp"
#include "API/UnixTypes.hpp"

namespace Syscall::VFS
{
    std::expected<isize, std::errno_t> SysRead(Syscall::Arguments& args);
    std::expected<isize, std::errno_t> SysWrite(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysOpen(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysClose(Syscall::Arguments& args);

    std::expected<i32, std::errno_t>   SysStat(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysFStat(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysLStat(Syscall::Arguments& args);

    std::expected<off_t, std::errno_t> SysLSeek(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysIoCtl(Syscall::Arguments& args);

    std::expected<i32, std::errno_t>   SysAccess(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysDup(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysDup2(Syscall::Arguments& args);

    std::expected<i32, std::errno_t>   SysFcntl(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysGetCwd(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysChDir(Syscall::Arguments& args);
    std::expected<i32, std::errno_t>   SysFChDir(Syscall::Arguments& args);

    [[clang::no_sanitize("alignment")]]
    std::expected<i32, std::errno_t> SysGetDents64(Syscall::Arguments& args);
    std::expected<i32, std::errno_t> SysOpenAt(Syscall::Arguments& args);
    std::expected<i32, std::errno_t> SysFStatAt(Syscall::Arguments& args);

    std::expected<i32, std::errno_t> SysDup3(Syscall::Arguments& args);
} // namespace Syscall::VFS
