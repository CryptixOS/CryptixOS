/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Access.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <functional>
#include <unordered_map>

namespace PCI
{
    using Enumerator = bool (*)(
        const DeviceAddress&); // std::function<bool(const DeviceAddress&)>;

    class HostController
    {
      public:
        HostController(const Domain& domain, Pointer address)
            : m_Domain(domain)
            , m_IoSpace(address ? new MemoryIoSpace(address) : nullptr)
            , m_Address(address)
        {
        }

        bool EnumerateDevices(Enumerator enumerator);

        template <std::unsigned_integral T>
        T Read(const DeviceAddress& address, RegisterOffset offset)
        {
            static_assert(sizeof(T) <= 4);
            return Read(address, static_cast<u8>(offset), sizeof(T));
        }
        template <std::unsigned_integral T>
        void Write(const DeviceAddress& address, RegisterOffset offset, T value)
        {
            static_assert(sizeof(T) <= 4);
            return Write(address, static_cast<u8>(offset), value, sizeof(T));
        }

      private:
        Domain                                   m_Domain;
        IoSpace*                                 m_IoSpace = nullptr;
        Pointer                                  m_Address;

        std::unordered_map<uintptr_t, uintptr_t> m_MappedBuses;

        uintptr_t GetAddress(const DeviceAddress& address, u64 offset);
        u64       Read(const DeviceAddress& address, u8 offset, u8 byteWidth);
        void      Write(const DeviceAddress& address, u8 offset, u64 value,
                        u8 byteWidth);

        u16       GetDeviceType(const DeviceAddress& address)
        {
            return (static_cast<u16>(
                        Read<u8>(address, RegisterOffset::eClassID))
                    << 8)
                 | Read<u8>(address, RegisterOffset::eSubClassID);
        }

        bool EnumerateRootBus(Enumerator enumerator);
        bool EnumerateBus(u8 bus, Enumerator enumerator);
        bool EnumerateSlot(const u8 bus, const u8 slot, Enumerator enumerator);
        bool EnumerateFunction(const DeviceAddress& address,
                               Enumerator           enumerator);
    };
}; // namespace PCI
