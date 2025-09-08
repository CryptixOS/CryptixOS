/*
 * Created by v1tr10l7 on 17.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>
#include <Drivers/PCI/Device.hpp>

namespace Virtio
{
    enum class ConfigurationType : u8
    {
        eCommon               = 1,
        eNotify               = 2,
        eISR                  = 3,
        eDevice               = 4,
        ePCICapabilitiesAccess = 5,
    };
    struct [[gnu::packed]] Configuration
    {
        ConfigurationType Type;
        u8                ResourceID;
        u32               Offset;
        u32               Length;
    };

    class Device : PCI::Device
    {
      public:
        Device(const PCI::DeviceAddress& address);

        void Initialize();
        void Reset();
        void Configure();

        void Start();
        void Stop();

      private:
        Configuration             m_CommonCfg;
        Configuration             m_NotifyCfg;
        Configuration             m_IsrCfg;
        UnorderedMap<u8, Pointer> m_RegisterBases;
        Vector<Configuration>     m_Configurations;

        void                      ParseCapabilities();
    };
}; // namespace Virtio
