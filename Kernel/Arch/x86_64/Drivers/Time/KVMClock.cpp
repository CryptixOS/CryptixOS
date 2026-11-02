/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#include <API/UnixTypes.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Core/Integer128.hpp>
#include <Prism/Containers/Array.hpp>

module Cryptix:Impl.Arch.x86_64.Drivers.Time.KVMClock;

import :API.DeviceIDs;
import :Arch.x86_64.CPU;
import :Arch.x86_64.Drivers.Time.KVMClock;
import :Time.Time;

namespace KVM
{
    ErrorOr<Clock*> Clock::Create()
    {
        if (!Supported()) return Error(ENODEV);

        return new Clock;
    }
    Clock::Clock()
        : ClockSource(Name(), API::DeviceMajor::MISCELLANEOUS, 232)
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
