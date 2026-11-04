/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Core/Singleton.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif

module Cryptix:Impl.Arch.x86_64.Drivers.IoApic;

import :Arch.CPU;
import :Arch.x86_64.CPU;
import :Arch.x86_64.IDT;
import :Arch.x86_64.Drivers.IoApic;
import :Arch.x86_64.Drivers.PIC;
import :Arch.x86_64.Drivers.Time.Lapic;
import :Firmware.ACPI.MADT;

static Vector<IoApic> s_IoApics{};

IoApic::IoApic(Pointer baseAddress, u32 gsiBase)
    : m_BaseAddressPhys(baseAddress)
    , m_GsiBase(gsiBase)
{
    m_BaseAddressVirt = m_BaseAddressPhys.ToHigherHalf<>();
    // VMM::GetKernelPageMap()->Map(m_BaseAddressVirt, m_BaseAddressPhys,
    //                              PageAttributes::eRW
    //                                  | PageAttributes::eWriteThrough);

    m_RegisterSelect  = m_BaseAddressVirt.As<volatile u32>();
    m_RegisterWindow  = m_BaseAddressVirt.Offset<volatile u32*>(0x10);

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
    I8259A::Instance()->MaskAllIRQs();
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

IoApicController::IoApicController() = default;

ErrorOr<void> IoApicController::Initialize()
{
    IoApic::Initialize();
    return {};
}
ErrorOr<void> IoApicController::Shutdown()
{
    for (auto& ioApic : IoApic::GetIoApics()) ioApic.MaskAllEntries();
    return {};
}

ErrorOr<void> IoApicController::Mask(u32 irq)
{
    if (!IoApic::IsAnyEnabled()) return Error(ENODEV);
    IoApic::SetIrqRedirect(CPU::Current()->LapicID, irq + 0x20, irq, false);

    return {};
}
ErrorOr<void> IoApicController::Unmask(u32 irq)
{
    if (!IoApic::IsAnyEnabled()) return Error(ENODEV);

    IoApic::SetIrqRedirect(CPU::Current()->LapicID, irq + 0x20, irq, true);
    return {};
}

ErrorOr<void> IoApicController::SendEOI(u32 irq)
{
    if (!IoApic::IsAnyEnabled()) return Error(ENODEV);
    Lapic::Instance()->SendEOI();

    return {};
}
