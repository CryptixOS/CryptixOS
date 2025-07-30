/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/Timers/KVMClock.hpp>

#include <Prism/Core/Integer128.hpp>
#include <Time/Time.hpp>

namespace KVM::Clock
{
    struct CTOS_PACKED ClockInfo
    {
        u32 Version;
        u32 Padding0;
        u64 TscTimestamp;
        u64 SystemTime;
        u32 TscToSystemMultiplier;
        i8  TscShift;
        u8  Flags;
        u8  Padding1[2];
    };
    static ClockInfo* s_ClockInfo = nullptr;
    static u64        s_Offset    = 0;

    static bool       Supported()
    {
        const auto base = CPU::KvmBase();
        if (!base) return false;

        CPU::ID id(base + 1, 0);
        if (!(id.rax & Bit(3))) return false;

        LogTrace("KVM: Clock supported");
        return true;
    }

    ErrorOr<Timestep> Read()
    {
        volatile auto pvclock = static_cast<ClockInfo*>(s_ClockInfo);

        while (pvclock->Version % 2) Arch::Pause();

        auto time = static_cast<u128>(CPU::RdTsc()) - pvclock->TscTimestamp;
        if (pvclock->TscShift >= 0) time <<= pvclock->TscShift;
        else time >>= -pvclock->TscShift;

        time = (time * pvclock->TscToSystemMultiplier) >> 32;
        time += pvclock->SystemTime;

        return Timestep((time - s_Offset).Low());
    }

    ErrorOr<void> Initialize()
    {
        if (!Supported()) return Error(ENODEV);

        s_ClockInfo = new ClockInfo;
        CPU::WriteMSR(0x4b564d01,
                      Pointer(s_ClockInfo).FromHigherHalf<u64>() | 1);

        auto kvmNs = TryOrRet(Read());
        s_Offset   = kvmNs - Time::GetReal().tv_nsec;
        return {};
    }
}; // namespace KVM::Clock
