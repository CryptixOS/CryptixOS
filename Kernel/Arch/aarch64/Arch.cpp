/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Arch/Arch.hpp>
#include <Compiler.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/termios.h>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>

module Cryptix:Impl.Arch.aarch64.Arch;

import :Arch.Arch;
import :Arch.aarch64.Drivers.GIC;

namespace Arch
{
    void                           Initialize() { Assert(GIC::Register()); }

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("msr daifclr, #0b1111; wfi");
    }
    void   Pause() { __asm__ volatile("isb" ::: "memory"); }

    time_t GetEpoch() { return 0; }

    void   EnableInterrupts() { __asm__ volatile("msr daifclr, #0b1111"); }
    void   DisableInterrupts() { __asm__ volatile("msr daifset, #0b1111"); }
    bool   ExchangeInterruptFlag(bool enabled)
    {
        u64 interruptsDisabled;
        __asm__ volatile("mrs %0, daif" : "=r"(interruptsDisabled));

        if (enabled) EnableInterrupts();
        else DisableInterrupts();
        return !interruptsDisabled;
    }

    void Reboot() {}
} // namespace Arch
