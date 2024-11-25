/*
 * Created by v1tr10l7 on 22.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "CPU.hpp"

namespace Syscall
{
    extern "C" void handleSyscall(CPUContext* ctx)
    {
        LogInfo("Syscall entry");
        for (;;) CPU::Halt();
    }
} // namespace Syscall
