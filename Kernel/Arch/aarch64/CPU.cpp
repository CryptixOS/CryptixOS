/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/CPU.hpp"

namespace CPU
{
    bool GetInterruptFlag()
    {
        u64 daif = 0;
        __asm__ volatile("mrs %0, daif" : "=r"(daif));

        return daif == 0;
    }
    void SetInterruptFlag(bool enabled)
    {
        if (enabled) __asm__ volatile("msr daifclr, #0b1111");
        else __asm__ volatile("msr daifset, #0b1111");
    }

    struct CPU;
    CPU*    GetCurrent() { return nullptr; }
    Thread* GetCurrentThread() { return nullptr; }

    void    PrepareThread(Thread* thread, uintptr_t pc)
    {
        (void)thread;
        (void)pc;
    }

    void SaveThread(Thread* thread, CPUContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }
    void LoadThread(Thread* thread, CPUContext* ctx)
    {
        (void)thread;
        (void)ctx;
    }
} // namespace CPU
