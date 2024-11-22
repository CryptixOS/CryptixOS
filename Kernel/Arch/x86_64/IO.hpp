/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Arch/x86_64/Types.hpp"

#include <concepts>

namespace IO
{
    template <std::unsigned_integral T>
    inline static void Out(word port, T value)
        requires(sizeof(T) <= sizeof(dword))
    {
        if constexpr (std::same_as<T, byte>)
            __asm__ volatile("outb %0, %1" : : "a"(value), "d"(port));
        else if constexpr (sizeof(T) == 2)
            __asm__ volatile("outw %0, %1" : : "a"(value), "d"(port));
        else if constexpr (sizeof(T) == 4)
            __asm__ volatile("outl %0, %1" : : "a"(value), "d"(port));
    }
    template <std::unsigned_integral T>
    inline static T In(word port)
        requires(sizeof(T) <= sizeof(dword))
    {
        T value = 0;
        if constexpr (sizeof(T) == 1)
            __asm__ volatile("inb %1, %0" : "=a"(value) : "d"(port));
        else if constexpr (sizeof(T) == 2)
            __asm__ volatile("inw %1, %0" : "=a"(value) : "d"(port));
        else if constexpr (sizeof(T) == 4)
            __asm__ volatile("inl %1, %0" : "=a"(value) : "d"(port));

        return value;
    }

    inline static void Wait() { Out<byte>(0x80, 0x00); }
    inline static void Delay(usize microseconds)
    {
        for (usize i = 0; i < microseconds; ++i) In<byte>(0x80);
    }
}; // namespace IO
