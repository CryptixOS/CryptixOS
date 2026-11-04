/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>

module Cryptix:Impl.Arch.aarch64.Drivers.Serial;

import :Drivers.Serial;
import :Memory.VMM;

namespace Serial
{
    constexpr const Pointer UART_BASE  = 0x9'000'000zu;
    static Pointer          s_UartVirt = UART_BASE;

    bool                    Initialize()
    {
        s_UartVirt = VMM::MapIoRegion(UART_BASE, PMM::PAGE_SIZE, true, 0);

        const char* string = "Serial: Initialized\n";
        for (const char* c = string; *c; c++) Write(*c);
        return true;
    }

    u8 Read()
    {
        while (*s_UartVirt.Offset<u16*>(0x18) & Bit(4))
            __asm__ volatile("isb" ::: "memory");

        return *s_UartVirt.As<u8>();
    }
    void Write(u8 byte)
    {
        while (*s_UartVirt.Offset<u16*>(0x18) & Bit(5))
            __asm__ volatile("isb" ::: "memory");

        *s_UartVirt.As<u8>() = byte;
    }
} // namespace Serial
