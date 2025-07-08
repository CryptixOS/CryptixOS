/*
 * Created by v1tr10l7 on 25.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Device.hpp>
#include <Prism/Containers/Vector.hpp>

namespace PCI
{
    using Enumerator = Delegate<bool(const DeviceAddress&)>;
    class Bus
    {
      public:
        Bus(class HostController* controller, u16 domain, u8 bus)
            : m_HostController(controller)
            , m_Domain(domain)
            , m_Bus(bus)
        {
            DetectDevices();
        }

        inline DeviceAddress Address() const { return {m_Domain, m_Bus, 0, 0}; }
        inline HostController* GetController() const
        {
            return m_HostController;
        }
        inline u16                    GetDomain() const { return m_Domain; }
        inline u8                     GetBus() const { return m_Domain; }

        inline const Vector<Device*>& GetDevices();
        void                          DetectDevices();

        Device*                       FindDeviceByID(const DeviceID& id);
        bool                          Enumerate(Enumerator enumerator);

      private:
        HostController* m_HostController = nullptr;
        u16             m_Domain;
        u8              m_Bus;

        Vector<Device*> m_Devices;
        Vector<Bus*>    m_ChildBridges;

        void            EnumerateFunction(DeviceAddress& address);
    };
}; // namespace PCI
