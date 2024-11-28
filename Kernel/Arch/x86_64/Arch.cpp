/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Arch/Arch.hpp"

#include "Common.hpp"

#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/Drivers/Timers/PIT.hpp"

namespace Arch
{
    void Initialize()
    {
        CPU::InitializeBSP();

        PIC::Remap(0x20, 0x28);
        PIC::MaskAllIRQs();

        PIT::Initialize();
    }

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("hlt");

        CTOS_ASSERT_NOT_REACHED();
    }
    void Pause() { __asm__ volatile("pause"); }
}; // namespace Arch
