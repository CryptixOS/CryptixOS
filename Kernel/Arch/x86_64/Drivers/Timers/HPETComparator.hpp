/*
 * Created by vitriol1744 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Time/HardwareTimer.hpp>

class HPETComparator final : public HardwareTimer
{
  public:
    HPETComparator() = default;
    HPETComparator(u8 number, u8 irq, bool periodicCapable, bool is64bitCapable)
    {
        (void)number;
        (void)irq;
        (void)periodicCapable;
        (void)is64bitCapable;
    }

    u8          GetComparatorIndex() const { return m_ComparatorIndex; }
    inline bool IsEnabled() const { return m_Enabled; }
    inline bool Is64BitCapable() const { return m_Is64BitCapable; }

    virtual ErrorOr<void> Start(u8 vector, TimeStep interval,
                                TimerMode mode) override
    {
        return Error(ENOSYS);
    }
    virtual void Stop() override {}

    virtual bool IsModeSupported(TimerMode mode) const override
    {
        return false;
    }
    virtual ErrorOr<void> SetMode(TimerMode) override { return Error(ENOSYS); }
    virtual bool          CanQueryRaw() const override { return false; }
    virtual u64           GetCurrentRaw() const override { return 0; }
    virtual u64           GetRawToNs(u64 raw) const override { return 0; }

    virtual void          ResetToDefaultTicksPerSeconds() const override {}
    virtual ErrorOr<void> SetFrequency(usize frequency) const override
    {
        return Error(ENOSYS);
    }

  private:
    [[maybe_unused]] u8        m_ComparatorIndex     = 0;
    [[maybe_unused]] TimerMode m_Mode                = TimerMode::eOneShot;
    [[maybe_unused]] bool      m_PeriodicCapable : 1 = false;
    [[maybe_unused]] bool      m_Enabled         : 1 = false;
    [[maybe_unused]] bool      m_Is64BitCapable  : 1 = false;
};
