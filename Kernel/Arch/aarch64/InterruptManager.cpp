/*
 * Created by v1tr10l7 on 24.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Common.hpp"

extern "C" void raiseSyncException()
{
    while (true) __asm__ volatile("wfi");
}

extern "C" void raiseFault()
{
    while (true) __asm__ volatile("wfi");
}

extern "C" void raiseIrq()
{
    while (true) __asm__ volatile("wfi");
}

namespace InterruptManager
{
    extern "C" char* exception_handlers;
    void             InstallExceptionHandlers()
    {
        __asm__ volatile("msr VBAR_EL1, %0" ::"r"(&exception_handlers));
        __asm__ volatile(
            // Save stack pointer
            "mov x1, sp\n"

            // Use SP_EL1 as the current stack pointer
            "mrs x0, SPSel\n"
            "orr x0, x0, #1\n"
            "msr SPSel, x0\n"

            // Restore stack pointer
            "mov sp, x1"
            :
            :
            : "x0", "x1", "memory");
    }
} // namespace InterruptManager
