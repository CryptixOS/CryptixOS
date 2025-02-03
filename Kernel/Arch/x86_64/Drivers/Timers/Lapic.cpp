/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/Timers/HPET.hpp>
#include <Arch/x86_64/Drivers/Timers/Lapic.hpp>
#include <Arch/x86_64/Drivers/Timers/PIT.hpp>

#include <Firmware/ACPI/ACPI.hpp>

#include <Memory/VMM.hpp>

constexpr u32                  LAPIC_EOI_ACK                      = 0x00;
CTOS_UNUSED constexpr u32      APIC_BASE_MSR                      = 0x1b;

CTOS_UNUSED constexpr u32      LAPIC_ID_REGISTER                  = 0x20;
CTOS_UNUSED constexpr u32      LAPIC_VERSION_REGISTER             = 0x30;
CTOS_UNUSED constexpr u32      LAPIC_TASK_PRIORITY_REGISTER       = 0x80;
constexpr u32                  LAPIC_EOI_REGISTER                 = 0xb0;
CTOS_UNUSED constexpr u32      LAPIC_LDR_REGISTER                 = 0xd0;
CTOS_UNUSED constexpr u32      LAPIC_DFR_REGISTER                 = 0xe0;
CTOS_UNUSED constexpr u32      LAPIC_SPURIOUS_REGISTER            = 0xf0;
CTOS_UNUSED constexpr u32      LAPIC_ESR_REGISTER                 = 0x280;
constexpr u32                  LAPIC_ICR_LOW_REGISTER             = 0x300;
constexpr u32                  LAPIC_ICR_HIGH_REGISTER            = 0x310;
constexpr u32                  LAPIC_TIMER_REGISTER               = 0x320;
constexpr u32                  LAPIC_LINT0_REGISTER               = 0x350;
constexpr u32                  LAPIC_LINT1_REGISTER               = 0x360;
constexpr u32                  LAPIC_TIMER_INITIAL_COUNT_REGISTER = 0x380;
constexpr u32                  LAPIC_TIMER_CURRENT_COUNT_REGISTER = 0x390;
constexpr u32                  LAPIC_TIMER_DIVIDER_REGISTER       = 0x3e0;
[[maybe_unused]] constexpr u32 LAPIC_X2APIC_ICR_REGISTER          = 0x830;

CTOS_UNUSED constexpr u32      LAPIC_TIMER_MASKED                 = 0x10000;
CTOS_UNUSED constexpr u32      LAPIC_TIMER_PERIODIC               = 0x20000;

bool                           CheckX2Apic()
{
    CPU::ID id;
    if (!id(1, 0)) return false;

    if (!(id.rcx & Bit(21))) return false;
    auto dmar = ACPI::GetTable("DMAR");
    if (!dmar) return true;

    return false;
}

void Lapic::Initialize()
{
    PIT::Initialize();
    LogTrace("LAPIC: Initializing for cpu #{}...", CPU::GetCurrent()->LapicID);
    x2apic = CheckX2Apic();
    LogInfo("LAPIC: X2APIC available: {}", x2apic);

    u64 base = CPU::ReadMSR(APIC_BASE_MSR) | Bit(11);
    if (x2apic) base |= Bit(10);

    CPU::WriteMSR(APIC_BASE_MSR, base);

    if (!x2apic)
    {
        auto physAddr = base & ~(0xfff);

        baseAddress   = VMM::AllocateSpace(0x1000, sizeof(u32), true);
        VMM::GetKernelPageMap()->Map(baseAddress, physAddr,
                                     PageAttributes::eRW
                                         | PageAttributes::eUncacheableStrong);
    }

    baseAddress = ToHigherHalfAddress<uintptr_t>(base & ~(0xfff));
    id          = x2apic ? Read(LAPIC_ID_REGISTER)
                         : (Read(LAPIC_ID_REGISTER) >> 24) & 0xff;

    Write(LAPIC_TASK_PRIORITY_REGISTER, 0x00);
    Write(LAPIC_SPURIOUS_REGISTER, 0xff | Bit(8));
    /*if (!x2apic)
    {
        Write(LAPIC_DFR_REGISTER, 0xf0000000);
        Write(LAPIC_LDR_REGISTER, Read(LAPIC_ID_REGISTER));
    }*/

    // for (const auto& nmi : MADT::GetLAPIC_NMIEntries())
    //     SetNmi(2, CPU::GetCurrent()->archID, nmi->processor, nmi->flags,
    //            nmi->lint);

    // Write(LAPIC_TIMER_REGISTER, 0x10000);
    // Write(LAPIC_TIMER_REGISTER, 0x20 | 0x20000);
    // Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    // Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, ticksPer10ms / 10);

    LogInfo("LAPIC: Initialized");
};

void Lapic::SendIpi(u32 flags, u32 id)
{
    if (!x2apic)
    {
        Write(LAPIC_ICR_HIGH_REGISTER, id << 24);
        Write(LAPIC_ICR_LOW_REGISTER, flags);
        return;
    }

    Write(LAPIC_ICR_LOW_REGISTER,
          (static_cast<u64>(id) << 32) | Bit(14) | flags);
}
void Lapic::SendEOI() { Write(LAPIC_EOI_REGISTER, LAPIC_EOI_ACK); }

void Lapic::Start(u8 vector, u64 ms, Mode mode)
{
    if (ticksPerMs == 0) CalibrateTimer();
    u64 ticks = ticksPerMs * ms;

    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    u64 value = Read(LAPIC_TIMER_REGISTER) & ~(3 << 17);

    value |= std::to_underlying(mode) << 17;
    value &= 0xffffff00;
    value |= vector;
    Write(LAPIC_TIMER_REGISTER, value);

    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, ticks ? ticks : 1);
    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~Bit(16));
}
void Lapic::Stop()
{
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0);
    Write(LAPIC_TIMER_REGISTER, 1 << 16);
}

u32 Lapic::Read(u32 reg)
{
    if (x2apic) return CPU::ReadMSR((reg >> 4) + 0x800);

    volatile auto ptr = reinterpret_cast<volatile u32*>(baseAddress);
    return *ptr;
}

void Lapic::Write(u32 reg, u64 value)
{
    if (x2apic) return CPU::WriteMSR((reg >> 4) + 0x800, value);

    volatile auto ptr = reinterpret_cast<volatile u32*>(baseAddress + reg);
    *ptr              = value;
}

void Lapic::CalibrateTimer()
{
    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0xffffffff);

    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~Bit(16));
    // HPET::GetDevices()[0].Sleep(10 * 1000);
    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) | Bit(16));

    ticksPerMs = (0xffffffff - Read(LAPIC_TIMER_CURRENT_COUNT_REGISTER)) / 10;
}
void Lapic::SetNmi(u8 vector, u8 currentCPUID, u8 cpuID, u16 flags, u8 lint)
{
    // A value of 0xFF means all the processors
    if (cpuID != 0xff && currentCPUID != cpuID) return;

    u32 nmi = 0x400 | vector;

    if (flags & 2) nmi |= 1 << 13;
    if (flags & 8) nmi |= 1 << 15;

    if (lint == 0) Write(LAPIC_LINT0_REGISTER, nmi);
    else if (lint == 1) Write(LAPIC_LINT1_REGISTER, nmi);
}
