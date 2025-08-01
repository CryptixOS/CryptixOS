/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/Time/KVMClock.hpp>

#include <Prism/Core/Integer128.hpp>
#include <Time/Time.hpp>

namespace KVM
{
    ErrorOr<Clock*> Clock::Create()
    {
        if (!Supported()) return Error(ENODEV);

        return new Clock;
    }
    Clock::Clock()
        : ClockSource(Name())
    {
    }

    StringView    Clock::Name() const PM_NOEXCEPT { return "kvm"_sv; }

    ErrorOr<void> Clock::Enable()
    {
        Memory::Fill(&m_Info, 0, sizeof(m_Info));
        CPU::WriteMSR(CPU::MSR::KVM_SYSTEM_TIME,
                      Pointer(&m_Info).FromHigherHalf<u64>() | Bit(0));

        auto kvmNs = TryOrRet(Now());
        m_Offset   = kvmNs - Time::GetReal().tv_nsec;

        return {};
    }
    ErrorOr<void> Clock::Disable()
    {
        CPU::WriteMSR(CPU::MSR::KVM_SYSTEM_TIME, 0);

        return {};
    }

    ErrorOr<Timestep> Clock::Now()
    {
        volatile auto pvclock = &m_Info;
        while (pvclock->Version % 2) Arch::Pause();

        auto time = static_cast<u128>(CPU::ReadTsc()) - pvclock->TscTimestamp;
        if (pvclock->TscShift >= 0) time <<= pvclock->TscShift;
        else time >>= -pvclock->TscShift;

        time = (time * pvclock->TscToSystemMultiplier) >> 32;
        time += pvclock->SystemTime;

        return Timestep((time - m_Offset).Low());
    }

    bool Clock::Supported()
    {
        static bool supported = []() -> bool
        {
            const auto base = CPU::KvmBase();
            if (!base) return false;

            CPU::ID id(base + 1, 0);
            if (!(id.rax & Bit(3))) return false;

            LogTrace("KVM: Querying clock supported");
            return true;
        }();

        return supported;
    }
}; // namespace KVM
