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
#include <Arch/x86_64/Drivers/Time/HPET.hpp>
#include <Arch/x86_64/Drivers/Time/PIT.hpp>
#include <Arch/x86_64/Drivers/Time/RTC.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Time/Time.hpp>

namespace InterruptManager
{
    void SetController(Ref<InterruptController> ctrl);
};
namespace Arch
{
    KERNEL_INIT_CODE
    void InstallExceptions()
    {
        GDT::Initialize();
        GDT::Load(0);

        IDT::Initialize();
        IDT::Load();
    }
    KERNEL_INIT_CODE
    void Initialize()
    {
        auto pic = I8259A::Instance();
        if (!pic->Initialize())
            LogError("Arch: Failed to initialize I8259A controller");

        auto ioApic = CreateRef<IoApicController>();
        if (!ioApic->Initialize())
        {
            LogError("Arch: Failed to initialize io apic");
            InterruptManager::SetController(pic);
        }
        else InterruptManager::SetController(ioApic);

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
        CPU::DisableInterrupts();

        u8 status = 0;
        do {
            status = IO::In<byte>(0x64);
            if (status & Bit(0)) IO::In<byte>(0x60);
        } while (status & Bit(1));

        IO::Out<byte>(0x64, 0xfe);
        IO::Out<word>(0x604, 0x2000);

        // if it failed, just triple fault
        struct CTOS_PACKED
        {
            u16      Limit = 0;
            upointer Base  = 0;
        } invalidIDT;
        __asm__ volatile("lidt %0" ::"m"(invalidIDT));

        for (;;) HaltAndCatchFire(nullptr, nullptr);
    }
    time_t GetEpoch() { return RTC::CurrentTime(); }
}; // namespace Arch
