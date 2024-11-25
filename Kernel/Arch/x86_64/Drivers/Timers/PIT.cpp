/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "PIT.hpp"

#include "Common.hpp"

#include "Arch/InterruptHandler.hpp"

#include "Arch/x86_64/CPU.hpp"
#include "Arch/x86_64/Drivers/PIC.hpp"
#include "Arch/x86_64/IDT.hpp"
#include "Arch/x86_64/IO.hpp"

#include <atomic>

namespace Scheduler
{
    void Schedule(CPUContext* ctx);
}

namespace PIT
{
    static constexpr const u64 PIT_FREQUENCY = 1000;
    static std::atomic<u64>    tick          = 0;
    static u8                  timerVector   = 0;
    static InterruptHandler*   handler       = nullptr;

    [[maybe_unused]]
    static void TimerTick(struct CPUContext* ctx)
    {
        LogInfo("TimerTick");
        Scheduler::Schedule(ctx);

        LogTrace("EOI");
        PIC::SendEOI(timerVector - 0x20);
        tick++;
    }

    void Sleep(u64 ms)
    {
        volatile u64 target = GetMilliseconds() + ms;
        while (GetMilliseconds() < target) Arch::Pause();
    }

    void Initialize()
    {
        static bool initialized = false;
        SetFrequency(PIT_FREQUENCY);

        if (initialized) return;
        LogTrace("PIT: Initializing...");
        initialized = true;

        handler     = IDT::AllocateHandler(0x20);
        handler->SetHandler(Scheduler::Schedule);
        handler->Reserve();

        timerVector = handler->GetInterruptVector();
        LogInfo("PIT: Initialized\nTimer vector = {} ", timerVector);
    }

    u8  GetInterruptVector() { return timerVector; }

    u64 GetCurrentCount()
    {
        IO::Out<byte>(0x43, 0x00);
        auto lo = IO::In<byte>(0x40);
        auto hi = IO::In<byte>(0x40) << 8;
        return static_cast<u16>(hi << 8) | lo;
    }
    u64  GetMilliseconds() { return tick * (1000 / PIT_FREQUENCY); }

    void SetFrequency(usize frequency)
    {
        u64 reloadValue = PIT_BASE_FREQUENCY / frequency;
        if (PIT_BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
        SetReloadValue(reloadValue);
    }
    void SetReloadValue(u16 reloadValue)
    {
        IO::Out<byte>(0x43, 0x34);
        IO::Out<byte>(0x40, static_cast<byte>(reloadValue));
        IO::Out<byte>(0x40, static_cast<byte>(reloadValue >> 8));
    }

}; // namespace PIT
