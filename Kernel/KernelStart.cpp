/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <icxxabi>
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "Common.hpp"

// stubs
void* operator new(size_t size) { return (void*)1; }
void* operator new(usize size, std::align_val_t) { return (void*)1; }
void* operator new[](size_t size) { return (void*)1; }
void* operator new[](size_t size, std::align_val_t) { return (void*)1; }
void  operator delete(void* p) noexcept {}
void  operator delete(void* p, std::align_val_t) noexcept {}
void  operator delete(void* p, usize) noexcept {}
void  operator delete[](void* p) noexcept {}
void  operator delete[](void* p, std::align_val_t) noexcept {}

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
    LogInfo("test");
    for (;;) hcf();
}
