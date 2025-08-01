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
#include <Arch/x86_64/Drivers/Time/HPET.hpp>
#include <Arch/x86_64/Drivers/Time/PIT.hpp>
#include <Arch/x86_64/Drivers/Time/RTC.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/HID/Ps2Controller.hpp>

#include <Time/Time.hpp>

namespace Arch
{
    KERNEL_INIT_CODE
    void Initialize()
    {
        auto status = I8259A::Instance().Initialize();
        if (!status) LogError("Arch: Failed to initialize I8259A controller");

        IoApic::Initialize();
        if (!HPET::DetectAndSetup()) LogError("HPET: Not Available");

        Assert(Time::RegisterTimer(PIT::Instance()));
        CPU::InitializeBSP();
        CPU::StartAPs();

        PCSpeaker::ToneOn(1000);
        IO::Delay(1000);
        PCSpeaker::ToneOff();

        LogInfo("Date: {:02}/{:02}/{:04} {:02}:{:02}:{:02}", RTC::GetDay(),
                RTC::GetMonth(), RTC::GetCentury() * 100 + RTC::GetYear(),
                RTC::GetHour(), RTC::GetMinute(), RTC::GetSecond());
    }

    KERNEL_INIT_CODE
    void ProbeDevices()
    {
        auto result = I8042Controller::Probe();
        if (!result)
            LogError("Arch({}): Failed to probe the i8042 controller",
                     CTOS_ARCH_STRING);
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
    time_t GetEpoch() { return RTC::CurrentTime(); }
}; // namespace Arch
