/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/IO.hpp>
#include <Drivers/PCI/HostController.hpp>

#include <Memory/MMIO.hpp>
#include <Memory/VMM.hpp>
#include <Utility/Logger.hpp>

namespace PCI
{
    template <typename T>
    static T Read(DeviceAddress addr, u8 offset)
    {
        u32 address = static_cast<u32>(0x80000000) | (addr.Bus << 16)
                    | (addr.Slot << 11) | (addr.Function << 8)
                    | (offset & 0xfc);

        IO::Out<u32>(PCI_CONFIG_ADDRESS, address);
        return IO::In<T>(PCI_CONFIG_DATA + (offset & 3));
    }
    template <typename T>
    static void Write(DeviceAddress addr, u8 offset, T data)
    {
        u32 address = static_cast<uint32_t>(0x80000000) | (addr.Bus << 16)
                    | (addr.Slot << 11) | (addr.Function << 8)
                    | (offset & 0xfc);
        IO::Out<u32>(PCI_CONFIG_ADDRESS, address);
        IO::Out<T>(PCI_CONFIG_DATA + (offset & 3), data);
    }

    bool HostController::EnumerateDevices(Enumerator enumerator)
    {
        return EnumerateRootBus(enumerator);
    }

    uintptr_t HostController::GetAddress(const DeviceAddress& address,
                                         u64                  offset)
    {
        uintptr_t phys
            = (m_Address.Raw() + ((address.Bus - m_Domain.BusStart) << 20)
               | (address.Slot << 15) | (address.Function << 12));

        if (m_MappedBuses.contains(phys)) return m_MappedBuses[phys] + offset;

        auto virt = VMM::AllocateSpace(1 << 20, sizeof(u32));
        VMM::GetKernelPageMap()->MapRange(
            virt, phys, 1 << 20,
            PageAttributes::eRW | PageAttributes::eUncacheableStrong);
        m_MappedBuses[phys] = virt;

        return virt + offset;
    }
    u64 HostController::Read(const DeviceAddress& address, u8 offset,
                             u8 byteWidth)
    {

        [[maybe_unused]] uintptr_t addr = GetAddress(address, offset);
        switch (byteWidth)
        {
            case 1:
                return /*MMIO::Read<u8>(
                    addr); //*/
                    ::PCI::Read<u32>(address, offset);
            case 2:
                return /*MMIO::Read<u16>(
                    addr); //*/
                    ::PCI::Read<u16>(address, offset);
            case 4:
                return /*MMIO::Read<u32>(
                    addr); //*/
                    ::PCI::Read<u8>(address, offset);

            default: break;
        }

        assert(false);
        return -1;
    }
    void HostController::Write(const DeviceAddress& address, u8 offset,
                               u64 value, u8 byteWidth)
    {
        [[maybe_unused]] uintptr_t addr = GetAddress(address, offset);
        switch (byteWidth)
        {
            case 1:
                return /*MMIO::Write<u8>(
                    addr, value); //*/
                    ::PCI::Write<u32>(address, offset, value);
            case 2:
                return /*MMIO::Write<u16>(
                    addr, value); //*/
                    ::PCI::Write<u16>(address, offset, value);
            case 4:
                return /*MMIO::Write<u32>(
                    addr, value); //*/
                    ::PCI::Write<u8>(address, offset, value);

            default: break;
        }

        assert(false);
    }

    bool HostController::EnumerateRootBus(Enumerator enumerator)
    {
        if ((Read<u8>(DeviceAddress(), RegisterOffset::eHeaderType) & Bit(7))
            == 0)
        {
            EnumerateBus(0, enumerator);
            return false;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                if (Read<u16>({0, 0, i}, RegisterOffset::eVendorID)
                    == PCI_INVALID)
                    continue;
                if (EnumerateBus(i, enumerator)) return true;
            }
        }

        return false;
    }
    bool HostController::EnumerateBus(u8 bus, Enumerator enumerator)
    {
        for (u8 slot = 0; slot < 32; ++slot)
            if (EnumerateSlot(bus, slot, enumerator)) return true;

        return false;
    }
    bool HostController::EnumerateSlot(const u8 bus, const u8 slot,
                                       Enumerator enumerator)
    {
        DeviceAddress address{};
        address.Bus      = bus;
        address.Slot     = slot;
        address.Function = 0;
        if (Read<u16>(address, RegisterOffset::eVendorID) == PCI_INVALID)
            return false;
        if ((Read<u8>(address, RegisterOffset::eHeaderType) & Bit(7)) == 0)
        {
            if (EnumerateFunction(address, enumerator)) return true;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                address.Function = i;
                if (Read<u16>(address, RegisterOffset::eVendorID)
                    != PCI_INVALID)
                {
                    if (EnumerateFunction(address, enumerator)) return true;
                }
            }
        }

        return false;
    }
    bool HostController::EnumerateFunction(const DeviceAddress& address,
                                           Enumerator           enumerator)
    {
        if (Read<u8>(address, RegisterOffset::eHeaderType) == 0)
        {
            LogDebug("Interrupt Line: {:#x}, Interrupt Pin: {:#x}",
                     Read<u8>(address, RegisterOffset::eInterruptLine),
                     Read<u8>(address, RegisterOffset::eInterruptPin));
        }

        if (enumerator(address)) return true;
        u16 type = GetDeviceType(address);
        if (type == ((0x06 << 8) | 0x04))
        {
            u8 secondaryBus = Read<u8>(address, RegisterOffset::eSecondaryBus);
            if (EnumerateBus(secondaryBus, enumerator)) return true;
        }
        return false;
    }
}; // namespace PCI
