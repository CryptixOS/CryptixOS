/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Singleton.hpp>
#include <Prism/Utility/Atomic.hpp>

#include <Time/HardwareTimer.hpp>

class Lapic : public HardwareTimer, public Singleton<Lapic>
{
  public:
    enum class Mode : u8
    {
        eOneshot     = 0,
        ePeriodic    = 1,
        eTscDeadline = 2,
    };

    void          Initialize();

    static bool   IsInitialized() { return s_Initialized; }

    StringView    ModelString() const override { return "Local APIC"_sv; }
    virtual usize InterruptVector() const override;
    virtual bool  IsCPULocal() const override { return true; }

    void          SendIpi(u32 flags, u32 id);
    void          SendEOI();

    static void   PanicIpi();

    ErrorOr<void> Start(TimerMode mode, Timestep interval) override;
    void          Stop() override;

    ErrorOr<void> SetFrequency(usize frequency) override
    {
        (void)frequency;
        return {};
    }

  private:
    static AtomicBool       s_Initialized;

    u32                     m_ID               = 0;
    uintptr_t               m_BaseAddress      = 0;
    bool                    m_X2Apic           = false;
    u64                     m_TicksPerMs       = 0;

    class InterruptHandler* m_InterruptHandler = nullptr;

    u32                     Read(u32 reg);
    void                    Write(u32 reg, u64 value);

    void                    CalibrateTimer();
    void SetNmi(u8 vector, u8 currentCPUID, u8 cpuID, u16 flags, u8 lint);

    static void Tick(CPUContext* context);
};
