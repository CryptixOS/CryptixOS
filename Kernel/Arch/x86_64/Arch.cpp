/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>

#include <Common.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PCSpeaker.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/Drivers/PS2Keyboard.hpp>
#include <Arch/x86_64/Drivers/Timers/HPET.hpp>
#include <Arch/x86_64/Drivers/Timers/PIT.hpp>
#include <Arch/x86_64/IO.hpp>

namespace Arch
{
    void Initialize()
    {
        PIC::Remap(0x20, 0x28);
        IoApic::Initialize();
        HPET::Initialize();

        CPU::InitializeBSP();

        // CPU::StartAPs();

        return;
        PCSpeaker::ToneOn(1000);
        IO::Delay(1000);
        PCSpeaker::ToneOff();

        PS2Keyboard::Initialize();
    }

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("hlt");

        CtosUnreachable();
    }
    void Pause() { __asm__ volatile("pause"); }
}; // namespace Arch
