/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Posix/time.h>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #define CTOS_ARCH_STRING "x86-64"
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #define CTOS_ARCH_STRING "aarch64"
#endif

class HardwareTimer;
namespace Arch
{
    KERNEL_INIT_CODE
    void Initialize();
    KERNEL_INIT_CODE
    void              ProbeDevices();

    [[noreturn]] void Halt();
    void              Pause();

    void              PowerOff();
    void              Reboot();

    KERNEL_INIT_CODE
    void   ProbeTimers(Vector<HardwareTimer*>& timers);
    time_t GetEpoch();
}; // namespace Arch
