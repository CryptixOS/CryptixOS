/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <ACPI/MADT.hpp>

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>

static std::vector<IoApic> s_IoApics{};

IoApic::IoApic(Pointer baseAddress, u32 gsiBase)
    : m_BaseAddress(baseAddress)
    , m_GsiBase(gsiBase)
    , m_RedirectionEntryCount((Read(1) & 0xff0000) >> 16)
{
    VMM::GetKernelPageMap()->Map(m_BaseAddress, m_BaseAddress,
                                 PageAttributes::eRW
                                     | PageAttributes::eWriteBack);
}

void IoApic::SetIRQRedirect(u32 lapicID, u8 vector, u8 irq, bool status)
{
    for (const auto& iso : MADT::GetIsoEntries())
    {
        if (iso->IrqSource != irq) continue;

        SetGSIRedirect(lapicID, vector, iso->Gsi, iso->Flags, status);
        return;
    }

    SetGSIRedirect(lapicID, vector, irq, 0, status);
}
void IoApic::SetGSIRedirect(u32 lapicID, u8 vector, u8 gsi, u16 flags,
                            bool status)
{
    IoApic& ioApic   = GetIoApicForGsi(gsi);

    u64     redirect = vector;
    if (flags & BIT(1)) redirect |= BIT(13);
    if (flags & BIT(3)) redirect |= BIT(15);

    if (!status) redirect |= BIT(16);
    redirect |= static_cast<u64>(lapicID) << 56;

    u32 ioRedirectTable = (gsi - ioApic.m_GsiBase) * 2 + 16;
    ioApic.Write(ioRedirectTable, static_cast<u32>(redirect));
    ioApic.Write(ioRedirectTable + 1, static_cast<u32>(redirect >> 32));
}

void IoApic::Initialize()
{
    if (!s_IoApics.empty())
    {
        LogWarn("IoApic::Initialize: Already initialized");
        return;
    }

    LogTrace("IoApic: Initializing...");
    PIC::MaskAllIRQs();
    LogTrace("IoApic: Legacy PIC disabled");

    LogTrace("IoApic: Enumerating controllers...");
    for (usize i = 0; const auto& entry : MADT::GetIoApicEntries())
    {
        LogInfo(
            "IoApic[{}]: Controller found: {{ BaseAddress: {:#x}, ID: {}, "
            "GsiBase: {}  }}",
            i++, entry->Address, entry->ApicID, entry->GsiBase);

        IoApic ioApic(entry->Address, entry->GsiBase);
        ioApic.MaskAll();

        s_IoApics.push_back(ioApic);
    }

    if (s_IoApics.empty())
    {
        LogError("IoApic: No controllers available");
        return;
    }

    LogTrace("IoApic: Controllers found: {}", s_IoApics.size());
    LogInfo("IoApic: Initialized");
}

IoApic& IoApic::GetIoApicForGsi(u32 gsi)
{
    for (auto& ioapic : s_IoApics)
    {
        if (gsi >= ioapic.m_GsiBase
            && gsi < ioapic.m_GsiBase + ioapic.GetGsiCount())
            return ioapic;
    }

    Panic("Cannot determine IO APIC from GSI {}", gsi);
}
