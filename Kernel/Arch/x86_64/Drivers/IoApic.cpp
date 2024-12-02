/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "IoApic.hpp"

#include "ACPI/MADT.hpp"
#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/IDT.hpp"

static std::vector<IoApic> ioapics;

namespace
{
    void Write(MADT::IOAPICEntry* ioApic, u32 reg, u32 value)
    {
        u64 base = Pointer(ioApic->address).ToHigherHalf<u64>();
        *reinterpret_cast<volatile u32*>(base)      = reg;
        *reinterpret_cast<volatile u32*>(base + 16) = value;
    }
    u32 Read(MADT::IOAPICEntry* ioApic, u32 reg)
    {
        u64 base = Pointer(ioApic->address).ToHigherHalf<u64>();
        *reinterpret_cast<volatile u32*>(base) = reg;
        return *reinterpret_cast<volatile u32*>(base + 16);
    }

    usize GetGSI_Count(MADT::IOAPICEntry* ioApic)
    {
        return ((Read(ioApic, 1) >> 16) & 0xff) + 1;
    }

    MADT::IOAPICEntry* IOApicFromGSI(u32 gsi)
    {
        for (const auto& entry : MADT::GetIOAPICEntries())
        {
            if (gsi >= entry->gsib && gsi <= entry->gsib + GetGSI_Count(entry))
                return entry;
        }
        Panic("Cannot determine IO APIC from GSI {}", gsi);
    }
} // namespace

void IoApic::SetIRQRedirect(u32 lapicID, u8 vector, u8 irq, bool status)
{
    std::vector<MADT::ISOEntry*> isos = MADT::GetISOEntries();
    for (usize i = 0; i < isos.size(); i++)
    {
        MADT::ISOEntry* iso = isos[i];
        if (iso->irqSource != irq) continue;

        SetGSIRedirect(lapicID, vector, iso->gsi, iso->flags, status);
        return;
    }

    SetGSIRedirect(lapicID, vector, irq, 0, status);
}
void IoApic::SetGSIRedirect(u32 lapicID, u8 vector, u8 gsi, u16 flags,
                            bool status)
{
    MADT::IOAPICEntry* ioApic   = IOApicFromGSI(gsi);

    u64                redirect = vector;
    if ((flags & BIT(1))) redirect |= BIT(13);

    if ((flags & BIT(3))) redirect |= BIT(15);

    if (!status) redirect |= BIT(16);

    redirect |= static_cast<u64>(lapicID) << 56;

    u32 ioRedirectTable = (gsi - ioApic->gsib) * 2 + 16;
    ::Write(ioApic, ioRedirectTable, static_cast<u32>(redirect));
    ::Write(ioApic, ioRedirectTable + 1, static_cast<u32>(redirect >> 32));
}

void IoApic::Initialize()
{
    PIC::MaskAllIRQs();

    for (const auto& ioapic : MADT::GetIOAPICEntries())
    {
        VMM::GetKernelPageMap()->Map(ioapic->address, ioapic->address,
                                     PageAttributes::eRW
                                         | PageAttributes::eWriteBack);
        for (usize i = 0; i < GetGSI_Count(ioapic); i++)
        {
            auto  entry = [](usize i) -> usize { return 0x10 + (i * 2); };

            usize maskedIrq
                = ::Read(ioapic, entry(i))
                | (static_cast<uint64_t>(::Read(ioapic, entry(i) + 0x01))
                   << 32);

            maskedIrq |= BIT(16);
            ::Write(ioapic, entry(i), static_cast<uint32_t>(maskedIrq));
            ::Write(ioapic, entry(i) + 0x01,
                    static_cast<uint32_t>(maskedIrq >> 32));
        }
    }

    LogTrace("IoApic: Initializing...");
    auto redirectIsaIrq = [](usize i)
    {
        for (const auto& entry : MADT::GetISOEntries())
        {
            if (entry->irqSource == i)
            {

                SetGSIRedirect(CPU::GetBspId(), entry->irqSource + 0x20,
                               entry->gsi, 0, false);
                IDT::GetHandler(entry->irqSource + 0x20)->Reserve();
                return;
            }
        }

        SetGSIRedirect(CPU::GetBspId(), i + 0x20, i, 0, false);
        IDT::GetHandler(i + 0x20)->Reserve();
    };

    if (MADT::LegacyPIC())
    {
        for (usize i = 0; i < 16; i++)
        {
            if (i == 2) continue;
            redirectIsaIrq(i);
        }
    }
}
