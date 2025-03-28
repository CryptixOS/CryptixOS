/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/utsname.h>
#include <API/Syscall.hpp>

namespace API::System
{
    enum class RebootCommand : isize
    {
        eRestart   = 0,
        eHalt      = 1,
        ePowerOff  = 2,
        eRestart2  = 3,
        eSuspend   = 4,
        eKexec     = 5,
        eUndefined = -1,
    };

    ErrorOr<isize>     Uname(utsname* out);
    ErrorOr<isize>     Reboot(RebootCommand cmd);
    ErrorOr<uintptr_t> SysPanic(const char* errorMessage);
} // namespace API::System
