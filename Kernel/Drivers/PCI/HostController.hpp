/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Device.hpp>
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
        HostController(const Domain& domain, PM::Pointer address)
            : m_Domain(domain)
            , m_Address(address)
        {
            if (m_Address)
            {
                m_AccessMechanism = new ECAM(m_Address, m_Domain.BusStart);
                return;
            }

            m_AccessMechanism = new LegacyAccessMechanism();
            Assert(m_AccessMechanism);
        }

        inline const Domain& GetDomain() const { return m_Domain; }

        bool                 EnumerateDevices(Enumerator enumerator);

        u32 Read(const DeviceAddress& dev, u32 offset, i32 accessSize)
        {
            Assert(m_AccessMechanism);
            return m_AccessMechanism->Read(dev, offset, accessSize);
        }
        void Write(const DeviceAddress& dev, u32 offset, u32 value,
                   i32 accessSize)
        {
            Assert(m_AccessMechanism);
            return m_AccessMechanism->Write(dev, offset, value, accessSize);
        }

        template <std::unsigned_integral T>
            requires(sizeof(T) <= 4)
        T Read(const DeviceAddress& address, u32 offset)
        {
            return Read(address, offset, sizeof(T));
        }
        template <std::unsigned_integral T>
            requires(sizeof(T) <= 4)
        void Write(const DeviceAddress& address, u32 offset, T value)
        {
            return Write(address, offset, value, sizeof(T));
        }

      private:
        Domain                                   m_Domain;
        AccessMechanism*                         m_AccessMechanism = nullptr;
        PM::Pointer                              m_Address;

        std::unordered_map<uintptr_t, uintptr_t> m_MappedBuses;

        uintptr_t GetAddress(const DeviceAddress& address, u64 offset);

        bool      EnumerateRootBus(Enumerator enumerator);
        bool      EnumerateBus(u8 bus, Enumerator enumerator);
        bool EnumerateSlot(const u8 bus, const u8 slot, Enumerator enumerator);
        bool EnumerateFunction(const DeviceAddress& address,
                               Enumerator           enumerator);
    };
}; // namespace PCI
