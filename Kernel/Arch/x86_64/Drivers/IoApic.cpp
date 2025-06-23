/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Arch/x86_64/Drivers/IoApic.hpp>
#include <Arch/x86_64/Drivers/PIC.hpp>

#include <Firmware/ACPI/MADT.hpp>

static Vector<IoApic> s_IoApics{};

IoApic::IoApic(Pointer baseAddress, u32 gsiBase)
    : m_BaseAddressPhys(baseAddress)
    , m_GsiBase(gsiBase)
{
    m_BaseAddressVirt = m_BaseAddressPhys.ToHigherHalf<>();
    VMM::GetKernelPageMap()->Map(m_BaseAddressPhys, m_BaseAddressVirt,
                                 PageAttributes::eRW
                                     | PageAttributes::eWriteBack);

    m_RegisterSelect = m_BaseAddressVirt.As<volatile u32>();
    m_RegisterWindow = m_BaseAddressVirt.Offset<volatile u32*>(0x10);

    m_RedirectionEntryCount
        = ((Read(IoApicRegister::eVersion) & 0xff0000) >> 16);
}

void IoApic::MaskGsi(u32 gsi) const
{
    IoApicRegister redirectionTableLow
        = (gsi - m_GsiBase) * 2 + IoApicRegister::eRedirectionTableLow;
    IoApicRegister redirectionTableHigh
        = (gsi - m_GsiBase) * 2 + IoApicRegister::eRedirectionTableHigh;

    u64 entry = (static_cast<u64>(Read(redirectionTableHigh)) << 32)
              | Read(redirectionTableLow);

    entry |= Bit(16);
    Write(redirectionTableLow, entry);
    Write(redirectionTableHigh, entry >> 32);
}

void IoApic::SetRedirectionEntry(u32 gsi, u64 entry)
{
    IoApicRegister redirectionTableLow
        = (gsi - m_GsiBase) * 2 + IoApicRegister::eRedirectionTableLow;
    IoApicRegister redirectionTableHigh
        = (gsi - m_GsiBase) * 2 + IoApicRegister::eRedirectionTableHigh;

    Write(redirectionTableLow, entry);
    Write(redirectionTableHigh, entry >> 32);
}

Vector<IoApic>& IoApic::GetIoApics() { return s_IoApics; }

void IoApic::SetIrqRedirect(u32 lapicID, u8 vector, u8 irq, bool status)
{
    for (const auto& iso : MADT::GetIsoEntries())
    {
        if (iso.IrqSource != irq) continue;

        SetGsiRedirect(lapicID, vector, iso.Gsi, iso.Flags, status);
        return;
    }

    SetGsiRedirect(lapicID, vector, irq, 0, status);
}
void IoApic::SetGsiRedirect(u32 lapicID, u8 vector, u8 gsi, u16 flags,
                            bool status)
{
    IoApic&                ioApic = GetIoApicForGsi(gsi);
    IoApicRedirectionEntry entry{};
    entry.Vector = vector;
    entry.PinPolarity
        = flags & Bit(1) ? PinPolarity::eActiveLow : PinPolarity::eActiveHigh;
    entry.TriggerMode = flags & Bit(3) ? InterruptTriggerMode::eLevel
                                       : InterruptTriggerMode::eEdge;

    if (!status) entry.Masked = true;
    entry.Destination = lapicID;

    ioApic.SetRedirectionEntry(gsi, entry);
}

void IoApic::Initialize()
{
    if (!s_IoApics.Empty())
    {
        LogWarn("IoApic::Initialize: Already initialized");
        return;
    }
    if (MADT::GetIoApicEntries().Empty())
    {
        LogError("IoApic: Not Available!");
        return;
    }

    LogTrace("IoApic: Initializing...");
    PIC::MaskAllIRQs();
    LogTrace("IoApic: Legacy PIC disabled");

    LogTrace("IoApic: Enumerating controllers...");
    for (usize i = 0; const auto& entry : MADT::GetIoApicEntries())
    {
        IoApic ioApic(entry.Address, entry.GsiBase);
        u8     version       = ioApic.Read(IoApicRegister::eVersion);
        u8     arbitrationID = ioApic.Read(IoApicRegister::eArbitrationID);

        LogInfo(
            "IoApic[{}]: Controller found: {{ BaseAddress: {:#x}, ID: {}, "
            "GsiBase: {}, Version: {}, ArbitrationID: {} }}",
            i++, entry.Address, entry.ApicID, entry.GsiBase, version,
            arbitrationID);

        ioApic.MaskAllEntries();
        ioApic.Enable();

        s_IoApics.PushBack(ioApic);
    }

    if (s_IoApics.Empty())
    {
        LogError("IoApic: No controllers available");
        return;
    }

    LogTrace("IoApic: Controllers found: {}", s_IoApics.Size());
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
