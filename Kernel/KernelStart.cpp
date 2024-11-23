/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <icxxabi>

#include "Common.hpp"

#include "Drivers/Serial.hpp"
#include "Memory/PMM.hpp"

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include "Arch/x86_64/GDT.hpp"
    #include "Arch/x86_64/IDT.hpp"
#endif

namespace Arch
{
    void Halt()
    {
        for (;;)
        {
#if CTOS_ARCH == CTOS_ARCH_X86_64
            __asm__("hlt");
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
            __asm__("wfi");
#endif
        }
    }
    void Pause()
    {
#if CTOS_ARCH == CTOS_ARCH_X86_64
        __asm__ volatile("pause");
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
        __asm__ volatile("isb" ::: "memory");
#endif
    }
    bool ExchangeInterruptFlag(bool enabled)
    {
        bool interruptsEnabled = false;
#if CTOS_ARCH == CTOS_ARCH_X86_64
        u64 rflags;
        __asm__ volatile(
            "pushfq\n\t"
            "pop %[rflags]"
            : [rflags] "=r"(rflags));

        interruptsEnabled = rflags & Bit(9);
        if (enabled) __asm__ volatile("sti");
        else __asm__ volatile("cli");
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
        u64 val;
        __asm__ volatile("mrs %0, daif" : "=r"(val));
        interruptsEnabled = !val;

        if (enabled) __asm__ volatile("msr daifclr, #0b1111");
        else __asm__ volatile("msr daifset, #0b1111");
#endif

        return interruptsEnabled;
    }
} // namespace Arch

extern "C" void kernelStart()
{
    Serial::Initialize();
    Logger::EnableOutput(LOG_OUTPUT_SERIAL);

    Logger::EnableOutput(LOG_OUTPUT_TERMINAL);

#if CTOS_ARCH == CTOS_ARCH_X86_64
    GDT::Initialize();
    GDT::Load(0);

    IDT::Initialize();
    IDT::Load();
#endif

    Assert(PMM::Initialize());
    icxxabi::Initialize();

    for (;;) Arch::Halt();
}
