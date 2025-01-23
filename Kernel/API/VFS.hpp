/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

namespace API::VFS
{
    ErrorOr<isize> SysRead(i32 fdNum, u8* out, usize bytes);
    ErrorOr<isize> SysWrite(i32 fdNum, const u8* in, usize bytes);
    ErrorOr<isize> SysClose(i32 fdNum);
} // namespace API::VFS
namespace Syscall::VFS
{
    ErrorOr<i32>   SysOpen(Syscall::Arguments& args);

    ErrorOr<i32>   SysStat(Syscall::Arguments& args);
    ErrorOr<i32>   SysFStat(Syscall::Arguments& args);
    ErrorOr<i32>   SysLStat(Syscall::Arguments& args);

    ErrorOr<off_t> SysLSeek(Syscall::Arguments& args);
    ErrorOr<i32>   SysIoCtl(Syscall::Arguments& args);

    ErrorOr<i32>   SysAccess(Syscall::Arguments& args);
    ErrorOr<i32>   SysDup(Syscall::Arguments& args);
    ErrorOr<i32>   SysDup2(Syscall::Arguments& args);

    ErrorOr<i32>   SysFcntl(Syscall::Arguments& args);
    ErrorOr<i32>   SysGetCwd(Syscall::Arguments& args);
    ErrorOr<i32>   SysChDir(Syscall::Arguments& args);
    ErrorOr<i32>   SysFChDir(Syscall::Arguments& args);
    ErrorOr<i32>   SysMkDir(Syscall::Arguments& args);

    [[clang::no_sanitize("alignment")]]
    ErrorOr<i32> SysGetDents64(Syscall::Arguments& args);
    ErrorOr<i32> SysOpenAt(Syscall::Arguments& args);
    ErrorOr<i32> SysMkDirAt(Syscall::Arguments& args);
    ErrorOr<i32> SysFStatAt(Syscall::Arguments& args);

    ErrorOr<i32> SysDup3(Syscall::Arguments& args);
} // namespace Syscall::VFS
