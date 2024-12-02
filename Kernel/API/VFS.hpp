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
    isize SysWrite(Syscall::Arguments& args);
    i32   SysOpen(Syscall::Arguments& args);
    isize SysRead(Syscall::Arguments& args);

    off_t SysLSeek(Syscall::Arguments& args);
    int   SysIoCtl(Syscall::Arguments& args);
} // namespace Syscall::VFS
