/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Compiler.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Containers/Vector.hpp>
#include <API/UnixTypes.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
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

module Cryptix:Impl.Arch.x86_64.Drivers.Time.HPET;

import :Arch.x86_64.Drivers.Time.HPET;
import :Firmware.ACPI.ACPI;
import :Memory.PMM;
import :Memory.VMM;

using namespace ACPI;

namespace HPET
{
    static Vector<TimerBlock> s_Devices;

    struct [[gnu::packed]] Comparator
    {
        u64       capabilities;
        u64       valueRegister;
        u64       fsbInterruptRouteRegister;
        const u64 reserved;
    };

    inline constexpr usize HPET_ENABLE     = Bit(0);
    inline constexpr usize HPET_LEG_RT_CNF = Bit(1);
    struct [[gnu::packed]] Entry
    {
        const u64   capabilities;
        u64         reserved1;
        u64         configuration;
        u64         reserved2;
        u64         interruptStatusRegister;
        u64         reserved3[25];
        u64         mainCounter;
        u64         reserved4;
        Comparator* comparators;
    };

    inline constexpr u8 GetRevision(const u64 capabilities)
    {
        return capabilities;
    }
    inline constexpr u8 GetTimerCount(const u64 capabilities)
    {
        return (((capabilities >> 8) & 0x1f) + 1);
    }
    inline constexpr u16 GetVendorID(const u64 capabilities)
    {
        return (capabilities >> 16) ^ 0xffff;
    }
    inline constexpr u32 GetTickPeriod(const u64 capabilities)
    {
        return capabilities >> 32;
    }

    static struct
    {
        u64  frequency   = 0;
        u16  vendorID    = 0;
        u16  minimumTick = 0;
        bool x64Capable  = false;
    } data;

    ErrorOr<TimerBlock*> TimerBlock::GetFromTable(Pointer hpetPhys)
    {
        LogInfo("HPET: Table #{} found at {:#x}", s_Devices.Size(),
                hpetPhys.Raw<u64>());

        auto virt = VMM::GetKernelPageMap()->MapIoRegion<SDTs::HPET>(hpetPhys);
        if (!virt) return Error(EFAULT);

        SDTs::HPET* table = virt.value();

        // NOTE: HPET is only usable from SystemMemory
        if (table->EventTimerBlock.AddressSpaceID
            != ACPI::AddressSpace::eSystemMemory)
        {
            LogError(
                "HPET: TimerBlock is in address space other than System "
                "Memory, so it cannot be used ",
                s_Devices.Size());

            return Error(ENODEV);
        }

        return new TimerBlock(table);
    }

    TimerBlock::TimerBlock(SDTs::HPET* table)
    {
        m_Entry = VMM::MapIoRegion<Entry>(table->EventTimerBlock.Address, true);
        Assert(m_Entry);

        if (!m_Entry) LogError("HPET: Timer entry address is invalid");
        LogInfo("HPET: Found device at {:#x}",
                reinterpret_cast<uintptr_t>(m_Entry));

        m_Entry->configuration &= ~HPET_LEG_RT_CNF;
        LogInfo("HPET: revision: {}", GetRevision(m_Entry->capabilities));
        LogInfo(
            "HPET: Legacy Replacement "
            "Route Capable: {}",
            (m_Entry->capabilities >> 15) & 1);

        LogInfo("HPET: Available comparators: {}",
                GetTimerCount(m_Entry->capabilities));
        data.vendorID   = GetVendorID(m_Entry->capabilities);
        data.x64Capable = m_Entry->capabilities & Bit(13);
        LogInfo("HPET: PCI vendorID = {}, x64Capable: {}", data.vendorID,
                data.x64Capable);

        tickPeriod = GetTickPeriod(m_Entry->capabilities);
        Assert(tickPeriod > 1'000'000 && tickPeriod <= 0x05f5e100);

        data.frequency = 0x38d7ea4c68000 / tickPeriod;
        LogInfo("HPET: Frequency is set to {:#x}", data.frequency);
        data.minimumTick     = table->MinimumTick;

        m_Entry->mainCounter = 0;
        m_Entry->configuration |= HPET_ENABLE;
        m_Entry->interruptStatusRegister = m_Entry->interruptStatusRegister;

        // TODO(v1tr10l7): Enumerate timers
        [[maybe_unused]] u32 gsiMask     = 0xffffffff;
        for (usize i = 0; i < GetTimerCount(m_Entry->capabilities); i++)
        {
            auto& comparator = m_Entry->comparators[i];
            (void)comparator;
        }
    }
    void TimerBlock::Disable() const { m_Entry->configuration &= ~HPET_ENABLE; }

    u64  TimerBlock::GetCounterValue() const { return m_Entry->mainCounter; }
    void TimerBlock::Sleep(u64 us) const
    {
        usize target = GetCounterValue() + (us * 1'000'000'000) / tickPeriod;
        while (GetCounterValue() < target);
    }

    ErrorOr<void> DetectAndSetup()
    {
        SDTs::HPET* hpetTable = nullptr;
        usize       i         = 0;

        while ((hpetTable = ACPI::GetTable<SDTs::HPET>("HPET", i++)) != nullptr)
        {
            auto hpet = TimerBlock::GetFromTable(
                Pointer(hpetTable).FromHigherHalf<>());
            if (!hpet) return Error(hpet.error());

            TimerBlock* timerBlock = hpet.value();
            s_Devices.PushBack(*timerBlock);
        }

        return {};
    }

    const Vector<TimerBlock>& GetDevices() { return s_Devices; }
}; // namespace HPET
