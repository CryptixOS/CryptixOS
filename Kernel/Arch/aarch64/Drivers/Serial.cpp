/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Drivers/Serial.hpp>
#include <Memory/VMM.hpp>

namespace Serial
{
    constexpr const uintptr_t UART_BASE  = 0x9'000'000;
    static uintptr_t          s_UartVirt = UART_BASE;

    bool                      Initialize()
    {
        s_UartVirt += BootInfo::GetHHDMOffset();

        return true;
    }

    u8 Read()
    {
        while (*reinterpret_cast<u16*>(s_UartVirt + 0x18) & Bit(4))
            __asm__ volatile("isb" ::: "memory");

        return *reinterpret_cast<u8*>(s_UartVirt);
    }
    void Write(u8 byte)
    {
        while (*reinterpret_cast<u16*>(s_UartVirt + 0x18) & Bit(5))
            __asm__ volatile("isb" ::: "memory");

        *reinterpret_cast<u8*>(s_UartVirt) = byte;
    }
} // namespace Serial
