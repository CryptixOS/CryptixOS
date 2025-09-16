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

    CTOS_ALWAYS_INLINE void EnableUserAccess() { Asm("straf" :: : "cc"); }
    CTOS_ALWAYS_INLINE void DisableUserAccess() { Asm("clraf" :: : "cc"); }
}; // namespace CPU
