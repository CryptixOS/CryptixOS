/*
 * Created by v1tr10l7 on 17.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Virtio/VirtioDevice.hpp>

namespace Virtio
{
    struct [[gnu::packed]] Capability
    {
        u8  CapabilityVendor;
        u8  CapabilityNext;
        u8  CapabilityLength;
        u8  ConfigType;
        u8  Bar;
        u8  Padding[3];
        u32 Offset;
        u32 Length;
    };
#define VIRTIO_PCI_CAP_COMMON_CFG 1
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
#define VIRTIO_PCI_CAP_ISR_CFG    3
#define VIRTIO_PCI_CAP_DEVICE_CFG 4

    Device::Device(const PCI::DeviceAddress& address)
        : PCI::Device(address)
    {
        EnableMemorySpace();
        ParseCapabilities();
    }

    void Device::ParseCapabilities()
    {
        u8 capabilityPointer
            = Read<u8>(PCI::RegisterOffset::eCapabilitiesPointer);
        u16 header = 0;

        for (; capabilityPointer; capabilityPointer = (header >> 8) & 0xfc)
        {
            header                         = ReadAt(capabilityPointer, 2);
            u8                capabilityID = header & 0xff;

            ConfigurationType configType   = static_cast<ConfigurationType>(
                ReadAt(capabilityPointer + 0x03, 1));
            if (configType == ConfigurationType::ePCICapabiltiesAccess)
                continue;
            else if (ToUnderlying(configType)
                         < ToUnderlying(ConfigurationType::eCommon)
                     || ToUnderlying(configType) > ToUnderlying(
                            ConfigurationType::ePCICapabiltiesAccess))
            {
                LogError("Virtio: Unknown capability configuration type => {}",
                         configType);
                return_err(, ENXIO);
            }

            u8 capabilityLength = ReadAt(capabilityPointer + 0x02, 1);
            if (capabilityLength < 0x10)
            {
                LogError("Virtio: Unexpected capability size => {}",
                         capabilityLength);
                break;
            }

            u8 resourceID = ReadAt(capabilityPointer + 0x04, 1);
            if (resourceID > 0x05)
            {
                LogError("Virtio: Unexpected capability BAR value => {}",
                         resourceID);
                break;
            }

            u32 offset = ReadAt(capabilityPointer + 0x08, 4);
            u32 length = ReadAt(capabilityPointer + 0x0c, 4);
            if (length == 0)
            {
                LogError(
                    "Virtio: Found configuration => {}, with invalid length of "
                    "0",
                    ToUnderlying(configType));
                break;
            }

            Configuration configuration;
            configuration.Type       = configType;
            configuration.ResourceID = resourceID;
            configuration.Offset     = offset;
            configuration.Length     = length;

            switch (configType)
            {
                case ConfigurationType::eCommon:
                {
                    auto bar    = GetBar(resourceID);
                    auto mapped = bar.Map(0);

                    m_CommonCfg = configuration;
                    m_RegisterBases[resourceID]
                        = mapped.Offset(configuration.Offset);
                    break;
                }
                case ConfigurationType::eNotify:
                {
                    m_NotifyCfg = configuration;
                    break;
                }
                case ConfigurationType::eISR:
                {
                    m_IsrCfg = configuration;
                    break;
                }

                default: break;
            }
            // m_NotifyMultiplier = ReadAt(capabilityPointer + 0x10, 4);
            m_Configurations.EmplaceBack(configType, resourceID, offset,
                                         length);
        }
    }
}; // namespace Virtio
