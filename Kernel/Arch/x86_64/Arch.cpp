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

    bool ExchangeInterruptFlag(bool enabled)
    {
        u64 rflags;
        __asm__ volatile(
            "pushfq\n\t"
            "pop %[rflags]"
            : [rflags] "=r"(rflags));

        if (enabled) __asm__ volatile("sti");
        else __asm__ volatile("cli");
        return rflags & Bit(9);
    }
}; // namespace Arch
