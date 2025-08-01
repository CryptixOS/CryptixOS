/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/InterruptManager.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>
#include <Arch/x86_64/Drivers/Time/Lapic.hpp>

#include <Firmware/ACPI/MADT.hpp>

namespace InterruptManager
{
    void InstallExceptions()
    {
        GDT::Initialize();
        GDT::Load(0);

        IDT::Initialize();
        IDT::Load();
    }
    InterruptHandler* AllocateHandler(u8 hint)
    {
        return IDT::AllocateHandler(hint);
    }

    void Mask(u8 vector)
    {
        if (IoApic::IsAnyEnabled())
            IoApic::SetIrqRedirect(CPU::Current()->LapicID, vector,
                                   vector - 0x20, false);
        else I8259A::Instance().Mask(vector - 0x20);
    }
    void Unmask(u8 vector)
    {
        if (IoApic::IsAnyEnabled())
            IoApic::SetIrqRedirect(CPU::Current()->LapicID, vector,
                                   vector - 0x20, true);
        else I8259A::Instance().Unmask(vector - 0x20);
    }

    void SendEOI(u8 vector)
    {
        if (IoApic::IsAnyEnabled()) Lapic::Instance()->SendEOI();
        else I8259A::Instance().SendEOI(vector);
    }
}; // namespace InterruptManager
