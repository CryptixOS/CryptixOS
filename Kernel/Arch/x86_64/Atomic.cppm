/*
 * Created by v1tr10l7 on 21.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Compiler.hpp>

export module Cryptix:Arch.x86_64.Atomic;

export import :Arch.x86_64.Types;

export template <typename T>
CTOS_ALWAYS_INLINE bool CompareExchange(volatile T* ptr, T oldValue, T newValue)
{
    u32           result{};
    volatile u32* target = reinterpret_cast<volatile u32*>(ptr);

    __asm__ volatile("lock; cmpxchgl %2, %1\n"
                     : "=a"(result), "+m"(*target)
                     : "r"(newValue),
                       "0"(*reinterpret_cast<volatile u32*>(&oldValue))
                     : "memory");

    return static_cast<T>(result) == oldValue;
}

export CTOS_ALWAYS_INLINE void MemoryBarrier()
{
    __asm__ volatile("mfence" ::: "memory");
}
export CTOS_ALWAYS_INLINE void ReadMemoryBarrier()
{
    __asm__ volatile("lfence" ::: "memory");
}
export CTOS_ALWAYS_INLINE void WriteMemoryBarrier()
{
    __asm__ volatile("sfence" ::: "memory");
}
