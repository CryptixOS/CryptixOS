/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/Arch.hpp"

#include "Common.hpp"

namespace Arch
{
    void                           Initialize() {}

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("hlt");

        CTOS_ASSERT_NOT_REACHED();
    }
    void Pause() { __asm__ volatile("pause"); }

    void EnableInterrupts() { __asm__ volatile("sti"); }
    void DisableInterrupts() { __asm__ volatile("cli"); }
    bool ExchangeInterruptFlag(bool enabled)
    {
        u64 rflags;
        __asm__ volatile(
            "pushfq\n\t"
            "pop %[rflags]"
            : [rflags] "=r"(rflags));

        if (enabled) EnableInterrupts();
        else DisableInterrupts();
        return rflags & Bit(9);
    }
}; // namespace Arch
