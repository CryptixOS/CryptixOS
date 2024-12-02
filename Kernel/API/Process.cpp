/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Process.hpp"

#include "Arch/InterruptGuard.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

namespace Syscall::Process
{
    int SysExit(Syscall::Arguments& args)
    {
        InterruptGuard(false);
        i32   code    = args.args[0];

        auto* process = CPU::GetCurrentThread()->parent;
        return process->Exit(code);
    }
}; // namespace Syscall::Process
