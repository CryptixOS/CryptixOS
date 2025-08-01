/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/Time/PIT.hpp>

#include <Arch/InterruptHandler.hpp>

#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Scheduler/Scheduler.hpp>
#include <Time/Time.hpp>

PIT* PIT::s_Instance = nullptr;

PIT::PIT()
    : HardwareTimer(Name())
{
    LogTrace("PIT: Initializing...");
    auto success = SetFrequency(FREQUENCY / 10);
    if (!success) LogWarn("PIT: Failed to set frequency to {:#x}", FREQUENCY);

    LogInfo("PIT: Frequency set to {}Hz", FREQUENCY);

    m_Handler = IDT::AllocateHandler(IRQ_HINT);
    m_Handler->SetHandler(Tick);
    m_Handler->Reserve();
    m_Handler->eoiFirst = true;

    m_TimerVector       = m_Handler->GetInterruptVector();
    LogInfo("PIT: Installed on interrupt gate #{:#x}", m_TimerVector);
}

void          PIT::Initialize() { s_Instance = Instance(); }
bool          PIT::IsInitialized() { return s_Instance != nullptr; }

ErrorOr<void> PIT::Start(TimerMode mode, Timestep interval)
{
    if (mode == TimerMode::eOneShot) m_CurrentMode = Mode::eOneShot;
    else if (mode == TimerMode::ePeriodic) m_CurrentMode = Mode::eSquareWave;
    else return Error(ENOSYS);

    usize reloadValue = (interval.Milliseconds() * BASE_FREQUENCY) / 3000;
    SetReloadValue(reloadValue);
    IO::Out<byte>(COMMAND, CHANNEL0_DATA | SEND_WORD | m_CurrentMode);

    InterruptManager::Unmask(m_TimerVector);

    return {};
}
void PIT::Stop()
{
    InterruptManager::Mask(m_TimerVector);
    SetReloadValue(0);
}

u8  PIT::GetInterruptVector() { return m_TimerVector; }

u64 PIT::GetCurrentCount()
{
    IO::Out<byte>(COMMAND, SELECT_CHANNEL0);
    auto lo = IO::In<byte>(CHANNEL0_DATA);
    auto hi = IO::In<byte>(CHANNEL0_DATA) << 8;
    return static_cast<u16>(hi << 8) | lo;
}

ErrorOr<void> PIT::SetFrequency(usize frequency)
{
    u64 reloadValue = BASE_FREQUENCY / frequency;
    if (BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
    SetReloadValue(reloadValue);

    return {};
}
void PIT::SetReloadValue(u16 reloadValue)
{
    IO::Out<byte>(COMMAND, SEND_WORD | m_CurrentMode);
    IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue));
    IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue >> 8));
}

void PIT::Tick(struct CPUContext* ctx)
{
    Instance()->m_Tick++;
    Time::Tick((1'000 / FREQUENCY) * 1'000'000);
}
