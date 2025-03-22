/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>

#include <Common.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/I8042Controller.hpp>
#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PCSpeaker.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/Drivers/Timers/HPET.hpp>
#include <Arch/x86_64/Drivers/Timers/PIT.hpp>
#include <Arch/x86_64/Drivers/Timers/RTC.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/HID/Ps2Controller.hpp>

#include <Time/HardwareTimer.hpp>

namespace Arch
{
    void Initialize()
    {
        PIC::Remap(0x20, 0x28);
        IoApic::Initialize();
        if (!HPET::DetectAndSetup()) LogError("HPET: Not Available");

        CPU::InitializeBSP();
        CPU::StartAPs();

        PCSpeaker::ToneOn(1000);
        IO::Delay(1000);
        PCSpeaker::ToneOff();

        I8042Controller::Probe();

        LogInfo("Date: {:02}/{:02}/{:04} {:02}:{:02}:{:02}", RTC::GetDay(),
                RTC::GetMonth(), RTC::GetCentury() * 100 + RTC::GetYear(),
                RTC::GetHour(), RTC::GetMinute(), RTC::GetSecond());
    }

    __attribute__((noreturn)) void Halt()
    {
        for (;;) __asm__ volatile("hlt");

        Panic("Shouldn't Reach");
        AssertNotReached();
    }
    void Pause() { __asm__ volatile("pause"); }

    void PowerOff() {}
    void Reboot()
    {
        if (!I8042Controller::GetInstance()->SendCommand(
                I8042Command::eResetCPU))
            return;
        IO::Out<word>(0x604, 0x2000);
    }

    void ProbeTimers(std::vector<HardwareTimer*>& timers)
    {
        if (Lapic::IsInitialized()) timers.push_back(Lapic::Instance());
        if (PIT::IsInitialized()) timers.push_back(PIT::Instance());
    }
    time_t GetEpoch() { return RTC::CurrentTime(); }
}; // namespace Arch
