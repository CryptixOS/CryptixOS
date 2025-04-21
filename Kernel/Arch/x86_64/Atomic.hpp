/*
 * Created by v1tr10l7 on 21.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/x86_64/Types.hpp>

#define CTOS_ALWAYS_INLINE [[gnu::always_inline]]
template <typename T>
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
