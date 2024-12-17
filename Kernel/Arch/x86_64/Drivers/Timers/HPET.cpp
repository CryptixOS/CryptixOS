/*
 * Created by vitriol1744 on 21.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "HPET.hpp"

#include "Firmware/ACPI/ACPI.hpp"

#include "Memory/VMM.hpp"

namespace HPET
{
    static std::vector<Device> devices;

    struct RegisterAddress
    {
        u8  addressSpaceID;
        u8  registerBitWidth;
        u8  registerBitOffset;
        u8  reserved;
        u64 address;
    } __attribute__((packed));
    struct Table
    {
        SDTHeader       header;
        u8              hardwareRevision;
        u8              comparatorCount   : 5;
        u8              counterSize       : 1;
        u8              reserved          : 1;
        u8              legacyReplacement : 1;
        u16             pciVendorID;
        RegisterAddress address;
        u8              hpetNumber;
        u16             minimumTick;
        u8              pageProtection;
    } __attribute__((packed));

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

    Device::Device(Table* table)
    {
        auto vaddr = VMM::AllocateSpace(sizeof(Entry), sizeof(u64), true);
        VMM::GetKernelPageMap()->Map(vaddr, table->address.address,
                                     PageAttributes::eRW);

        entry = reinterpret_cast<Entry*>(vaddr);
        if (!entry) LogError("HPET: Timer entry address is invalid");
        LogInfo("HPET: Found device at {:#x}",
                reinterpret_cast<uintptr_t>(entry));

        entry->configuration &= ~HPET_LEG_RT_CNF;
        LogInfo("HPET: revision: {}", GetRevision(entry->capabilities));
        LogInfo(
            "HPET: Legacy Replacement "
            "Route Capable: {}",
            (entry->capabilities >> 15) & 1);

        LogInfo("HPET: Available comparators: {}",
                GetTimerCount(entry->capabilities));
        data.vendorID   = GetVendorID(entry->capabilities);
        data.x64Capable = entry->capabilities & BIT(13);
        LogInfo("HPET: PCI vendorID = {}, x64Capable: {}", data.vendorID,
                data.x64Capable);

        tickPeriod = GetTickPeriod(entry->capabilities);
        Assert(tickPeriod > 1'000'000 && tickPeriod <= 0x05f5e100);

        data.frequency = 0x38d7ea4c68000 / tickPeriod;
        LogInfo("HPET: Frequency is set to {:#x}", data.frequency);
        data.minimumTick   = table->minimumTick;

        entry->mainCounter = 0;
        entry->configuration |= HPET_ENABLE;
        entry->interruptStatusRegister = entry->interruptStatusRegister;

        // TODO(v1tr10l7): Enumerate timers
        [[maybe_unused]] u32 gsiMask   = 0xffffffff;
        for (usize i = 0; i < GetTimerCount(entry->capabilities); i++)
        {
            auto& comparator = entry->comparators[i];
            (void)comparator;
        }
    }
    void Device::Disable() const { entry->configuration &= ~HPET_ENABLE; }

    u64  Device::GetCounterValue() const { return entry->mainCounter; }
    void Device::Sleep(u64 us) const
    {
        usize target = GetCounterValue() + (us * 1'000'000'000) / tickPeriod;
        while (GetCounterValue() < target)
            ;
    }

    void Initialize()
    {
        Table* table = nullptr;
        usize  index = 0;

        LogTrace("HPET: Enumerating devices...");
        while ((table = ACPI::GetTable<Table>("HPET", index++)) != nullptr)
            devices.push_back(table);
    }

    const std::vector<Device>& GetDevices() { return devices; }
}; // namespace HPET
