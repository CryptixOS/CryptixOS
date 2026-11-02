/*
 * Created by v1tr10l7 on 17.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

module Cryptix:Impl.Drivers.Virtio.VirtioDevice;

import :Drivers.Virtio.VirtioDevice;

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
            if (configType == ConfigurationType::ePCICapabilitiesAccess)
                continue;
            else if (ToUnderlying(configType)
                         < ToUnderlying(ConfigurationType::eCommon)
                     || ToUnderlying(configType) > ToUnderlying(
                            ConfigurationType::ePCICapabilitiesAccess))
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
