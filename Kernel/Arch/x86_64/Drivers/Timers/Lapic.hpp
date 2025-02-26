/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Time/HardwareTimer.hpp>

class Lapic : public HardwareTimer
{
  public:
    enum class Mode : u8
    {
        eOneshot     = 0,
        ePeriodic    = 1,
        eTscDeadline = 2,
    };

    void          Initialize();
    void          SendIpi(u32 flags, u32 id);
    void          SendEOI();

    static void   PanicIpi();

    ErrorOr<void> Start(u8 vector, TimeStep interval, TimerMode mode) override;
    void          Stop() override;

    virtual bool  IsModeSupported(TimerMode mode) const override
    {
        return false;
    }
    virtual ErrorOr<void> SetMode(TimerMode mode) override
    {
        return Error(ENOSYS);
    }
    virtual bool  CanQueryRaw() const override { return false; }
    virtual u64   GetCurrentRaw() const override { return 0; }
    virtual u64   GetRawToNs(u64 raw) const override { return 0; }

    virtual void  ResetToDefaultTicksPerSeconds() const override { ; }

    ErrorOr<void> SetFrequency(usize frequency) const override
    {
        (void)frequency;
        return {};
    }

  private:
    u32       id          = 0;
    uintptr_t baseAddress = 0;
    bool      x2apic      = false;
    u64       ticksPerMs  = 0;

    u32       Read(u32 reg);
    void      Write(u32 reg, u64 value);

    void      CalibrateTimer();
    void      SetNmi(u8 vector, u8 currentCPUID, u8 cpuID, u16 flags, u8 lint);
};
