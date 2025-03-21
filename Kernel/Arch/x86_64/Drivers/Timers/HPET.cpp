/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/Timers/HPET.hpp>

#include <Firmware/ACPI/ACPI.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Prism/Memory/Pointer.hpp>

using namespace ACPI;

namespace HPET
{
    using Prism::Pointer;
    static std::vector<TimerBlock> s_Devices;

    struct Comparator
    {
        u64       capabilities;
        u64       valueRegister;
        u64       fsbInterruptRouteRegister;
        const u64 reserved;
    } __attribute__((packed));

    inline constexpr usize HPET_ENABLE     = BIT(0);
    inline constexpr usize HPET_LEG_RT_CNF = BIT(1);
    struct Entry
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
    } __attribute__((packed));

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
        LogInfo("HPET: Table #{} found at {:#x}", s_Devices.size(),
                hpetPhys.Raw<u64>());

        auto virt = VMM::GetKernelPageMap()->MapIoRegion<SDTs::HPET>(hpetPhys);
        if (!virt) return Error(EFAULT);

        SDTs::HPET* table = virt.value();

        // NOTE: HPET is only usable from SystemMemory
        if (table->EventTimerBlock.AddressSpaceID
            != AddressSpace::eSystemMemory)
        {
            LogError(
                "HPET: TimerBlock is in address space other than System "
                "Memory, so it cannot be used ",
                s_Devices.size());

            return Error(ENODEV);
        }

        return new TimerBlock(table);
    }

    TimerBlock::TimerBlock(SDTs::HPET* table)
    {
        auto virt = VMM::GetKernelPageMap()->MapIoRegion<Entry>(
            table->EventTimerBlock.Address);
        Assert(virt);
        m_Entry = virt.value();

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
        data.x64Capable = m_Entry->capabilities & BIT(13);
        LogInfo("HPET: PCI vendorID = {}, x64Capable: {}", data.vendorID,
                data.x64Capable);

        tickPeriod = GetTickPeriod(m_Entry->capabilities);
        Assert(tickPeriod > 1'000'000 && tickPeriod <= 0x05f5e100);

        data.frequency = 0x38d7ea4c68000 / tickPeriod;
        LogInfo("HPET: Frequency is set to {:#x}", data.frequency);
        data.minimumTick = table->MinimumTick;

        LogDebug("Enabling hpet");
        m_Entry->mainCounter = 0;
        m_Entry->configuration |= HPET_ENABLE;
        m_Entry->interruptStatusRegister = m_Entry->interruptStatusRegister;

        // TODO(v1tr10l7): Enumerate timers
        [[maybe_unused]]
        u32 gsiMask
            = 0xffffffff;
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
            s_Devices.push_back(*timerBlock);
        }

        return {};
    }

    const std::vector<TimerBlock>& GetDevices() { return s_Devices; }
}; // namespace HPET
