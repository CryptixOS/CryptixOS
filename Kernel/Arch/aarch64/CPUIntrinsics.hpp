/*
 * Created by v1tr10l7 on 09.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>

namespace CPU
{
#define Asm(...) __asm__ volatile(__VA_ARGS__)
#define ReadSystemRegister(register)                                           \
    ({                                                                         \
        u64 value;                                                             \
        Asm("mrs %0, " CtStringify(register) : "=r"(value));                   \
        value;                                                                 \
    })
#define WriteSystemRegister(newValue, register)                                \
    do {                                                                       \
        u64 value = static_cast<u64>(newValue);                                \
        Asm("msr " CtStringify(register) ", %x0" : : "rZ"(value));             \
    } while (0)

    CTOS_ALWAYS_INLINE void Halt() { Asm("msr daifclr, #0b1111; wfi"); }
    CTOS_ALWAYS_INLINE void Pause() { Asm("isb" :: : "memory"); }

    CTOS_NODISCARD CTOS_ALWAYS_INLINE bool InterruptsEnabled()
    {
        u64 daif = 0;
        Asm("mrs %0, daif" : "=r"(daif));

        return daif == 0;
    }

    CTOS_ALWAYS_INLINE void EnableInterrupts() { Asm("msr daifclr, #0b1111"); }
    CTOS_ALWAYS_INLINE void DisableInterrupts() { Asm("msr daifset, #0b1111"); }
    CTOS_ALWAYS_INLINE bool ExchangeInterruptFlag(bool enabled)
    {
        u64 interruptsDisabled;
        Asm("mrs %0, daif" : "=r"(interruptsDisabled));

        if (enabled) EnableInterrupts();
        else DisableInterrupts();
        return !interruptsDisabled;
    }

    constexpr usize         TTBR_ASID_MASK          = 0xffffzu << 48zu;
    constexpr usize         PAGE_SIZE               = 0x1000;
    constexpr usize         RESERVED_SWAPPER_OFFSET = PAGE_SIZE;

    CTOS_ALWAYS_INLINE void DisableUserAccess()
    {
        bool irqEnabled = ExchangeInterruptFlag(false);
        u64  ttbr       = ReadSystemRegister(ttbr1_el1);
        ttbr &= ~TTBR_ASID_MASK;

        WriteSystemRegister(ttbr - RESERVED_SWAPPER_OFFSET, ttbr0_el1);
        WriteSystemRegister(ttbr, ttbr1_el1);

        Asm("isb" : : : "memory");
        ExchangeInterruptFlag(irqEnabled);
    }
    CTOS_ALWAYS_INLINE void EnableUserAccess()
    {
        /*
         * Disable interrupts to avoid preemption between reading the 'ttbr0'
         * variable and the MSR. A context switch could trigger an ASID
         * roll-over and an update of 'ttbr0'.
         */
        bool irqFlag = ExchangeInterruptFlag(false);
        // FIXME(v1tr10l7): actually read ttbr0 from current thread
        u64  ttbr0   = 0;

        u64  ttbr1   = ReadSystemRegister(ttbr1_el1);
        ttbr1 &= ~TTBR_ASID_MASK;
        ttbr1 |= ttbr0 & TTBR_ASID_MASK;
        WriteSystemRegister(ttbr1, ttbr1_el1);

        WriteSystemRegister(ttbr0, ttbr0_el1);
        Asm("isb" : : : "memory");
        ExchangeInterruptFlag(irqFlag);
    }
}; // namespace CPU
