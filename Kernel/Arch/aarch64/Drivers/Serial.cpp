/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Drivers/Serial.hpp"

#include "Utility/BootInfo.hpp"

namespace Serial
{
    constexpr const uintptr_t UART_BASE = 0x9000000;
    static uintptr_t s_UartAddress      = UART_BASE + BootInfo::GetHHDMOffset();

    bool             Initialize() { return true; }

    u8               Read()
    {
        while (*reinterpret_cast<u16*>(s_UartAddress + 0x18) & Bit(4))
            asm volatile("isb" ::: "memory");
        // Arch::Pause();

        return *reinterpret_cast<u8*>(s_UartAddress);
    }
    void Write(u8 byte)
    {
        while (*reinterpret_cast<u16*>(s_UartAddress + 0x18) & Bit(5))
            asm volatile("isb" ::: "memory");

        *reinterpret_cast<u8*>(s_UartAddress) = byte;
    }
} // namespace Serial
