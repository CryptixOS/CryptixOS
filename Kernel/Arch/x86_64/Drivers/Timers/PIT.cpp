/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/Timers/PIT.hpp>

#include <Arch/InterruptHandler.hpp>

#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/IO.hpp>

#include <atomic>

namespace Scheduler
{
    void Schedule(CPUContext* ctx);
}

namespace PIT
{
    namespace
    {
        constexpr usize   FREQUENCY     = 1000;
        // NOTE(v1tr10l7): When using PIC, we cannot choose irq number of PIT,
        // and it will have to be IRQ, only IoApic allows to redirect irqs
        constexpr usize   IRQ_HINT      = 0x20;
        std::atomic<u64>  s_Tick        = 0;
        u8                s_TimerVector = 0;
        InterruptHandler* s_Handler     = nullptr;
        usize             s_CurrentMode = Mode::RATE;

        void              TimerTick(struct CPUContext* ctx)
        {
            Scheduler::Schedule(ctx);
            s_Tick++;
            // PIC::SendEOI(s_TimerVector - 0x20);
        }
    } // namespace

    void Initialize()
    {
        static bool initialized = false;
        if (initialized) return;

        LogTrace("PIT: Initializing...");
        SetFrequency(FREQUENCY / 10);
        LogInfo("PIT: Frequency set to {}Hz", FREQUENCY);

        s_Handler = IDT::AllocateHandler(IRQ_HINT);
        s_Handler->SetHandler(TimerTick);
        s_Handler->Reserve();
        s_Handler->eoiFirst = true;

        s_TimerVector       = s_Handler->GetInterruptVector();
        LogInfo("PIT: Installed on interrupt gate #{:#x}", s_TimerVector);
        LogInfo("PIT: Initialized\nTimer vector = {} ", s_TimerVector);
        initialized = true;
    }

    void Start(usize mode, usize ms)
    {
        s_CurrentMode     = mode;

        // reloadValue = (ms×frequency) / 3000
        usize reloadValue = (ms * BASE_FREQUENCY) / 3000;
        SetReloadValue(reloadValue);
        IO::Out<byte>(COMMAND, CHANNEL0_DATA | SEND_WORD | mode);
        // InterruptManager::Unmask(s_TimerVector);
    }
    void Stop()
    {
        InterruptManager::Mask(s_TimerVector);
        SetReloadValue(0);
    }

    u8  GetInterruptVector() { return s_TimerVector; }

    u64 GetCurrentCount()
    {
        IO::Out<byte>(COMMAND, SELECT_CHANNEL0);
        auto lo = IO::In<byte>(CHANNEL0_DATA);
        auto hi = IO::In<byte>(CHANNEL0_DATA) << 8;
        return static_cast<u16>(hi << 8) | lo;
    }
    u64  GetMilliseconds() { return s_Tick * (1000z / FREQUENCY); }

    void SetFrequency(usize frequency)
    {
        u64 reloadValue = BASE_FREQUENCY / frequency;
        if (BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
        SetReloadValue(reloadValue);
    }
    void SetReloadValue(u16 reloadValue)
    {
        IO::Out<byte>(COMMAND, SEND_WORD | s_CurrentMode);
        IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue));
        IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue >> 8));
    }

    void Sleep(u64 ms)
    {
        volatile u64 target = GetMilliseconds() + ms;
        while (GetMilliseconds() < target) Arch::Pause();
    }

}; // namespace PIT
