/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <cstddef>
#include <cstdint>
#include <icxxabi>

#include <memory>

#include "Common.hpp"

#include "Drivers/Serial.hpp"
#include "Memory/PMM.hpp"

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include "Arch/x86_64/GDT.hpp"
#endif

namespace Arch
{
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

static void hcf()
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

extern "C" void kernelStart()
{
    Framebuffer* framebuffer = BootInfo::GetFramebuffer();

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    for (size_t i = 0; i < 100; i++)
    {
        volatile uint32_t* fb_ptr
            = reinterpret_cast<volatile u32*>(framebuffer->address);
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }

    Serial::Initialize();
    Logger::EnableOutput(LOG_OUTPUT_SERIAL);

    Assert(PMM::Initialize());
    icxxabi::Initialize();

#if CTOS_ARCH == CTOS_ARCH_X86_64
    GDT::Initialize();
    GDT::Load(0);
#endif
    for (;;) hcf();
}
