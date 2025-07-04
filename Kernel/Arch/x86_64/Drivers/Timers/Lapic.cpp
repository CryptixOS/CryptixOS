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
#include <Scheduler/Scheduler.hpp>

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
[[maybe_unused]] constexpr u32 LAPIC_m_X2Apic_ICR_REGISTER        = 0x830;

CTOS_UNUSED constexpr u32      LAPIC_TIMER_MASKED                 = 0x10000;
CTOS_UNUSED constexpr u32      LAPIC_TIMER_PERIODIC               = 0x20000;

AtomicBool                     Lapic::s_Initialized               = false;

bool                           Checkm_X2Apic()
{
    CPU::ID m_ID;
    if (!m_ID(1, 0)) return false;

    if (!(m_ID.rcx & Bit(21))) return false;
    auto dmar = ACPI::GetTable("DMAR");
    if (!dmar) return true;

    return false;
}

void Lapic::Initialize()
{
    PIT::Initialize();
    LogTrace("LAPIC: Initializing for cpu #{}...", CPU::GetCurrent()->LapicID);
    m_X2Apic = Checkm_X2Apic();
    LogInfo("LAPIC: m_X2Apic available: {}", m_X2Apic);

    u64 base = CPU::ReadMSR(APIC_BASE_MSR) | Bit(11);
    if (m_X2Apic) base |= Bit(10);

    CPU::WriteMSR(APIC_BASE_MSR, base);

    if (!m_X2Apic)
    {
        auto physAddr = base & ~(0xfff);

        m_BaseAddress = VMM::AllocateSpace(0x1000, sizeof(u32), true);
        VMM::GetKernelPageMap()->Map(m_BaseAddress, physAddr,
                                     PageAttributes::eRW
                                         | PageAttributes::eUncacheableStrong);
    }

    m_BaseAddress = ToHigherHalfAddress<uintptr_t>(base & ~(0xfff));
    m_ID          = m_X2Apic ? Read(LAPIC_ID_REGISTER)
                             : (Read(LAPIC_ID_REGISTER) >> 24) & 0xff;

    Write(LAPIC_TASK_PRIORITY_REGISTER, 0x00);
    Write(LAPIC_SPURIOUS_REGISTER, 0xff | Bit(8));
    /*if (!m_X2Apic)
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

    if (!s_Initialized)
    {
        s_Initialized      = true;
        m_InterruptHandler = IDT::AllocateHandler();
        m_InterruptHandler->Reserve();
        LogInfo("LAPIC: Interrupt Vector: {}",
                m_InterruptHandler->GetInterruptVector());
        m_InterruptHandler->SetHandler(Tick);
    }
    LogInfo("LAPIC: Initialized");
};

void Lapic::SendIpi(u32 flags, u32 m_ID)
{
    if (!m_X2Apic)
    {
        Write(LAPIC_ICR_HIGH_REGISTER, m_ID << 24);
        Write(LAPIC_ICR_LOW_REGISTER, flags);
        return;
    }

    Write(LAPIC_ICR_LOW_REGISTER,
          (static_cast<u64>(m_ID) << 32) | Bit(14) | flags);
}
void          Lapic::SendEOI() { Write(LAPIC_EOI_REGISTER, LAPIC_EOI_ACK); }

ErrorOr<void> Lapic::Start(TimerMode tm, TimeStep interval)
{
    Mode  mode   = tm == TimerMode::eOneShot ? Mode::eOneshot : Mode::ePeriodic;
    u8    vector = m_InterruptHandler->GetInterruptVector();
    usize ms     = interval.Milliseconds();

    if (m_TicksPerMs == 0) CalibrateTimer();
    u64 ticks = m_TicksPerMs * ms;

    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    u64 value = Read(LAPIC_TIMER_REGISTER) & ~(3 << 17);

    value |= std::to_underlying(mode) << 17;
    value &= 0xffffff00;
    value |= vector;
    Write(LAPIC_TIMER_REGISTER, value);

    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, ticks ? ticks : 1);
    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~Bit(16));

    return {};
}
void Lapic::Stop()
{
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0);
    Write(LAPIC_TIMER_REGISTER, 1 << 16);
}

u32 Lapic::Read(u32 reg)
{
    if (m_X2Apic) return CPU::ReadMSR((reg >> 4) + 0x800);

    volatile auto ptr = reinterpret_cast<volatile u32*>(m_BaseAddress);
    return *ptr;
}

void Lapic::Write(u32 reg, u64 value)
{
    if (m_X2Apic) return CPU::WriteMSR((reg >> 4) + 0x800, value);

    volatile auto ptr = reinterpret_cast<volatile u32*>(m_BaseAddress + reg);
    *ptr              = value;
}

void Lapic::CalibrateTimer()
{
    Write(LAPIC_TIMER_DIVIDER_REGISTER, 0x03);
    Write(LAPIC_TIMER_INITIAL_COUNT_REGISTER, 0xffffffff);

    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) & ~Bit(16));
    // HPET::GetDevices()[0].Sleep(10 * 1000);
    Write(LAPIC_TIMER_REGISTER, Read(LAPIC_TIMER_REGISTER) | Bit(16));

    m_TicksPerMs = (0xffffffff - Read(LAPIC_TIMER_CURRENT_COUNT_REGISTER)) / 10;
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

void Lapic::Tick(CPUContext* context)
{
    if (Instance()->m_OnTickCallback) Instance()->m_OnTickCallback(context);
}
