/*
 * Created by v1tr10l7 on 24.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Singleton.hpp>
#include <Prism/Core/Types.hpp>

#include <Time/HardwareTimer.hpp>

class PIT : public HardwareTimer, public PM::Singleton<PIT>
{
  public:
    PIT();

    static void      Initialize();

    std::string_view GetModelString() const override { return "i8253"; }

    ErrorOr<void> Start(TimerMode mode, TimeStep interval, u8 vector) override;
    void          Stop() override;

    u8            GetInterruptVector();

    u64           GetCurrentCount();
    u64           GetMilliseconds();

    ErrorOr<void> SetFrequency(usize frequency) override;
    void          SetReloadValue(u16 reloadValue);

    void          Sleep(u64 ms);

    static constexpr usize BASE_FREQUENCY  = 1193182ull;
    static constexpr usize SEND_WORD       = 0x30;

    static constexpr usize CHANNEL0_DATA   = 0x40;
    static constexpr usize CHANNEL1_DATA   = 0x41;
    static constexpr usize CHANNEL2_DATA   = 0x42;
    static constexpr usize COMMAND         = 0x43;

    static constexpr usize SELECT_CHANNEL0 = 0x00;
    static constexpr usize SELECT_CHANNEL1 = 0x40;
    static constexpr usize SELECT_CHANNEL2 = 0x80;

    enum Mode
    {
        eCountDown  = 0x00,
        eOneShot    = 0x02,
        eRate       = 0x04,
        eSquareWave = 0x06,
    };

  private:
    static PIT*             s_Instance;
    class InterruptHandler* m_Handler     = nullptr;
    // NOTE(v1tr10l7): When using PIC, we cannot choose irq number of PIT,
    // and it will have to be IRQ, only IoApic allows to redirect irqs
    u8                      m_TimerVector = 0;
    usize                   m_CurrentMode = Mode::eRate;
    std::atomic<u64>        m_Tick        = 0;

    static constexpr usize  FREQUENCY     = 1000;
    static constexpr usize  IRQ_HINT      = 0x20;

    static void             Tick(struct CPUContext* ctx);
}; // namespace PIT
