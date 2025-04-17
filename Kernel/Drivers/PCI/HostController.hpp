/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Bus.hpp>
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Prism/Delegate.hpp>

#include <functional>
#include <unordered_map>

#include <uacpi/resources.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

namespace PCI
{
    using Enumerator = Delegate<bool(const DeviceAddress&)>;

    class HostController
    {
      public:
        HostController(const Domain& domain, Pointer address)
            : m_Domain(domain)
            , m_Address(address)
        {
        }

        void Initialize();
        void InitializeIrqRoutes();

        struct IrqRoute
        {
            u64  Gsi;
            i32  Device;
            i32  Function;
            u8   Pin;
            bool EdgeTriggered;
            bool ActiveHigh;
        };

        inline const Domain&   GetDomain() const { return m_Domain; }
        inline const auto&     GetIrqRoutes() const { return s_IrqRoutes; }
        inline const IrqRoute* FindIrqRoute(i32 device, u8 pin) const
        {
            for (const auto& entry : s_IrqRoutes)
                if (entry.Device == device && entry.Pin == pin) return &entry;
            return nullptr;
        }

        bool EnumerateDevices(Enumerator enumerator);

        u32  Read(const DeviceAddress& dev, u32 offset, i32 accessSize)
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
        Vector<Bus*>                             m_RootBuses;

        Pointer                                  m_Address;
        uacpi_namespace_node*                    m_RootNode;

        static std::vector<IrqRoute>             s_IrqRoutes;

        std::unordered_map<uintptr_t, uintptr_t> m_MappedBuses;

        uintptr_t GetAddress(const DeviceAddress& address, u64 offset);

        bool      EnumerateRootBus(Enumerator enumerator);
    };
}; // namespace PCI
