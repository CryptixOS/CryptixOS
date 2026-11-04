/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/Core/Concepts.hpp>

export module Cryptix:Arch.x86_64.IO;

export import :Arch.x86_64.Types;

namespace IO
{
    export template <UnsignedIntegral T>
    inline void Out(word port, T value)
        requires(sizeof(T) <= sizeof(dword))
    {
        if constexpr (SameAs<T, byte>)
            __asm__ volatile("outb %0, %1" : : "a"(value), "d"(port));
        else if constexpr (sizeof(T) == 2)
            __asm__ volatile("outw %0, %1" : : "a"(value), "d"(port));
        else if constexpr (sizeof(T) == 4)
            __asm__ volatile("outl %0, %1" : : "a"(value), "d"(port));
    }
    export template <UnsignedIntegral T>
    inline T In(word port)
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

    export inline void Wait() { Out<byte>(0x80, 0x00); }
    export inline void Delay(usize microseconds)
    {
        for (usize i = 0; i < microseconds; ++i) In<byte>(0x80);
    }
}; // namespace IO
