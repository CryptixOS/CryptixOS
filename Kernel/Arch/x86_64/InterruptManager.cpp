/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/InterruptManager.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>

#include <Firmware/ACPI/MADT.hpp>

namespace InterruptManager
{
    void InstallExceptionHandlers()
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

    void Mask(u8 irq)
    {
        if (IoApic::IsAnyEnabled())
            IoApic::SetIrqRedirect(CPU::GetCurrent()->LapicID, irq + 0x20, irq,
                                   false);
        else PIC::MaskIRQ(irq);
    }
    void Unmask(u8 irq)
    {
        if (IoApic::IsAnyEnabled())
            IoApic::SetIrqRedirect(CPU::GetCurrent()->LapicID, irq + 0x20, irq,
                                   true);
        else PIC::UnmaskIRQ(irq);
    }

    void SendEOI(u8 vector)
    {
        if (IoApic::IsAnyEnabled()) CPU::GetCurrent()->Lapic.SendEOI();
        else PIC::SendEOI(vector);
    }
}; // namespace InterruptManager
