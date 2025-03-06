/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>

namespace Arch
{
    void                           Initialize() {}

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("msr daifclr, #0b1111; wfi");
    }
    void  Pause() { __asm__ volatile("isb" ::: "memory"); }

    usize GetEpoch() { return 0; }

    void  EnableInterrupts() { __asm__ volatile("msr daifclr, #0b1111"); }
    void  DisableInterrupts() { __asm__ volatile("msr daifset, #0b1111"); }
    bool  ExchangeInterruptFlag(bool enabled)
    {
        u64 interruptsDisabled;
        __asm__ volatile("mrs %0, daif" : "=r"(interruptsDisabled));

        if (enabled) EnableInterrupts();
        else DisableInterrupts();
        return !interruptsDisabled;
    }

    void ProbeTimers(std::vector<HardwareTimer*>&) {}

    void Reboot() {}
} // namespace Arch
