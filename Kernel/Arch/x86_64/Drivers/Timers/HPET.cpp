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

#include <Prism/Math.hpp>
#include <Prism/Pointer.hpp>

using namespace ACPI;

constexpr usize NanoSecondPeriodToHertz(usize x) { return 1000000000 / x; }
constexpr usize HertzToMegaHertz(usize x) { return (x / 1000000); }

namespace HPET
{
    using Prism::Pointer;
    static std::vector<TimerBlock> s_Devices;

    enum class Attributes : u32
    {
        eRevisionID                      = 0xff,
        eComparatorCount                 = 0xf00,
        e64BitCounter                    = Bit(13),
        eLegacyReplacementMappingCapable = Bit(15),
        eVendorID                        = 0xff00,
    };

    u64 operator&(Attributes lhs, const Attributes rhs)
    {
        u64 result = static_cast<u64>(lhs) & static_cast<u64>(rhs);

        return result;
    }
    enum class Configuration : u32
    {
        eEnableMainCounter              = Bit(0),
        eLegacyReplacementMappingEnable = Bit(1),
    };

    Configuration operator~(Configuration lhs)
    {
        u64 result = ~static_cast<u64>(lhs);

        return static_cast<Configuration>(result);
    }
    Configuration operator|=(Configuration lhs, Configuration rhs)
    {
        u64 result = static_cast<u64>(lhs) | static_cast<u64>(rhs);

        return static_cast<Configuration>(result);
    }
    Configuration operator&=(Configuration lhs, Configuration rhs)
    {
        u64 result = static_cast<u64>(lhs) & static_cast<u64>(rhs);

        return static_cast<Configuration>(result);
    }

    struct [[gnu::packed]] CapabilityRegister
    {
        volatile Attributes    Attributes;
        volatile Configuration MainCounterTickPeriod;
        u64                    Reserved;
    };

    enum class ComparatorCapabilities : u64
    {
        eGeneratesLevelTriggeredInterrupts = Bit(1),
        eEnablesInterrupts                 = Bit(2),
        eEnablePeriodicTimer               = Bit(3),
        ePeriodicTimerSupport              = Bit(4),
        e64BitTimer                        = Bit(5),
        eAllowSoftwareToSetAccumulator     = Bit(6),
        eEnable32BitMode                   = Bit(8),
        eEnableIoApicRouting               = 0xf << 9,
        eEnableFsbInterruptMapping         = Bit(14),
        eSupportFsbMapping                 = Bit(15),
    };

    u64 operator&(ComparatorCapabilities lhs, ComparatorCapabilities rhs)
    {
        u64 result = static_cast<u64>(lhs) & static_cast<u64>(rhs);

        return result;
    }

    struct [[gnu::packed]] Comparator
    {
        ComparatorCapabilities Capabilities;
        u64                    ValueRegister;
        u64                    FsbInterruptRouteRegister;
        const u64              Reserved;
    };

    struct [[gnu::packed]] Entry
    {
        const CapabilityRegister Capabilities;
        u64                      Reserved1;
        Configuration            Configuration;
        u64                      Reserved2;
        u64                      InterruptStatusRegister;
        u64                      Reserved3[25];
        u64                      MainCounter;
        u64                      Reserved4;
        Comparator*              Comparators;
    };

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

        m_VendorID      = table->PCI_VendorID;
        m_MinimimumTick = table->MinimumTick;
        LogInfo("HPET: Revision: {}",
                m_Entry->Capabilities.Attributes & Attributes::eRevisionID);
        LogInfo("HPET: Minimum clock tick - {}", m_MinimimumTick);

        m_TimerCount
            = m_Entry->Capabilities.Attributes & Attributes::eComparatorCount;
        m_X64Capable
            = m_Entry->Capabilities.Attributes & Attributes::e64BitCounter;
        LogInfo("HPET: Available comparators: {}", m_TimerCount);
        LogInfo("HPET: Main Counter size: {}",
                m_X64Capable ? "64-bit" : "32-bit");
        LogInfo(
            "HPET: Legacy Replacement "
            "Route Capable: {}",
            m_Entry->Capabilities.Attributes
                & Attributes::eLegacyReplacementMappingCapable);

        return; // TODO(v1tr10l7): Initialize all comparators
        for (usize i = 0; i < m_TimerCount; i++)
        {
            bool capable64Bit = m_Entry->Comparators[i].Capabilities
                              & ComparatorCapabilities::e64BitTimer;
            LogInfo("HPET: Comparator[{}]: {{ size: {}, mode: {} }}", i,
                    capable64Bit ? "64bit" : "32bit",
                    ((!capable64Bit
                              || (m_Entry->Comparators[i].Capabilities
                                  & ComparatorCapabilities::eEnable32BitMode)
                          ? "32-bit"
                          : "64-bit")));
        }
        Assert(m_TimerCount >= 2);
        Disable();

        m_Frequency = NanoSecondPeriodToHertz(RawCounterTicksToNs(1));
        LogInfo("HPET: frequency: {} Hz ({} MHz) resolution: {} ns",
                m_Frequency, HertzToMegaHertz(m_Frequency),
                RawCounterTicksToNs(1));

        Assert(GetCounterValue() <= ABSOLUTE_MAXIMUM_COUNTER_TICK_PERIOD);

        m_Entry->MainCounter = 0;
        if (m_Entry->Capabilities.Attributes
            & Attributes::eLegacyReplacementMappingCapable)
            m_Entry->Configuration
                |= Configuration::eLegacyReplacementMappingEnable;

        m_Comparators.push_back(
            HPETComparator(0, 0,
                           m_Entry->Comparators[0].Capabilities
                               & ComparatorCapabilities::ePeriodicTimerSupport,
                           m_Entry->Comparators[0].Capabilities
                               & ComparatorCapabilities::e64BitTimer));

        m_Comparators.push_back(
            HPETComparator(1, 8,
                           m_Entry->Comparators[1].Capabilities
                               & ComparatorCapabilities::ePeriodicTimerSupport,
                           m_Entry->Comparators[1].Capabilities
                               & ComparatorCapabilities::e64BitTimer));

        Enable();
    }

    void TimerBlock::Enable() const
    {
        m_Entry->Configuration |= Configuration::eEnableMainCounter;
    }
    void TimerBlock::Disable() const
    {
        m_Entry->Configuration &= ~Configuration::eEnableMainCounter;
    }
    u64  TimerBlock::GetCounterValue() const { return m_Entry->MainCounter; }
    void TimerBlock::Sleep(u64 us) const
    {
        // usize target = GetCounterValue() + (us * 1'000'000'000) / tickPeriod;
        // while (GetCounterValue() < target)
        //    ;
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
