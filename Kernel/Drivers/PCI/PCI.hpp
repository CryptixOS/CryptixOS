/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Utility/Types.hpp>

namespace PCI
{
    constexpr usize PCI_CONFIG_ADDRESS = 0x0cf8;
    constexpr usize PCI_CONFIG_DATA    = 0x0cfc;

    enum class VendorID
    {
        eQemu  = 0x1234,
        eIntel = 0x8086,
    };

    constexpr u16 PCI_INVALID = 0xffff;

    enum class HeaderType
    {
        eDevice        = 0x00,
        ePciBridge     = 0x01,
        eCardBusBridge = 0x02,
    };
    enum class ClassID
    {
        eUnclassified                     = 0x00,
        eMassStorageController            = 0x01,
        eNetworkController                = 0x02,
        eDisplayController                = 0x03,
        eMultimediaController             = 0x04,
        eMemoryController                 = 0x05,
        eBridge                           = 0x06,
        eSimpleCommunicationController    = 0x07,
        eBaseSystemPeripheral             = 0x08,
        eInputDeviceController            = 0x09,
        eDockingStation                   = 0x0a,
        eProcessor                        = 0x0b,
        eSerialBusController              = 0x0c,
        eWirelessController               = 0x0d,
        eIntelligentController            = 0x0e,
        eSatelliteCommunicationController = 0x0f,
        eEncryptionController             = 0x10,
        eSignalProcessingController       = 0x11,
        eProcessingAccelerator            = 0x12,
        eNonEssentialInstrumentation      = 0x13,
        eCoprocessor                      = 0x40,
        eUnassigned                       = 0xff,
    };
    enum class SubClassID
    {
        eSataController = 0x06,
        eNVMeController = 0x08,
    };
    enum class ProgIf
    {
        eAHCIv1     = 0x01,
        eNVMeXpress = 0x02,
    };

    struct DeviceAddress
    {
        u32 Domain   = 0;
        u8  Bus      = 0;
        u8  Slot     = 0;
        u8  Function = 0;
    };
    struct DeviceID
    {
        u16 VendorID = 0;
        u16 ID       = 0;

        operator bool() const { return !VendorID && !ID; }
        auto operator<=>(const DeviceID&) const = default;
    };

    struct Domain
    {
        Domain() = delete;
        Domain(u32 id, u8 busStart, u8 busEnd)
            : ID(id)
            , BusStart(busStart)
            , BusEnd(busEnd)
        {
        }

        u32 ID;
        u8  BusStart;
        u8  BusEnd;
    };
    struct Device
    {
    };

    void Initialize();
}; // namespace PCI
