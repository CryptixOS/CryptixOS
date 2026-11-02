/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Arch/Arch.hpp>
#include <Compiler.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

export module Cryptix:Arch.Arch;

export class HardwareTimer;
export namespace Arch
{
    KERNEL_INIT_SECTION
    void InstallExceptions();
    KERNEL_INIT_SECTION
    void               Initialize();

    CTOS_NORETURN void Halt();
    void               Pause();

    void               PowerOff();
    void               Reboot();

    time_t             GetEpoch();
}; // namespace Arch
