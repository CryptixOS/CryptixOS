/*
 * Created by v1tr10l7 on 25.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

namespace PCI
{
    enum class VendorID : u16
    {
        eQemu   = 0x1234,
        eRedHat = 0x1b36,
        eIntel  = 0x8086,
    };
    enum class Status : u16
    {
        eInterruptStatus       = Bit(3),
        eCapabilitiesList      = Bit(4),
        e66MhzCapable          = Bit(5),
        eFastBackToBackCapable = Bit(7),
        eMasterDataParityError = Bit(8),
        eDevSelTimingLow       = Bit(9),
        eDevSelTimingHigh      = Bit(10),
        eSignaledTargetAbort   = Bit(11),
        eReceivedTargetAbort   = Bit(12),
        eReceivedMasterAbort   = Bit(13),
        eSignaledSystemError   = Bit(14),
        eDetectedParityError   = Bit(15),
    };
    enum class Command : u16
    {
        eEnableIOSpace            = Bit(0),
        eEnableMMIO               = Bit(1),
        eEnableBusMaster          = Bit(2),
        eMonitorSpecialCycles     = Bit(3),
        eMemoryWriteAndInvalidate = Bit(4),
        eVgaPaletteSnoop          = Bit(5),
        eParityErrorResponse      = Bit(6),
        eSerrEnable               = Bit(8),
        eFastBackToBack           = Bit(9),
        eInterruptDisable         = Bit(10),
    };
    enum class ClassID : u8
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
    enum class SubClassID : u8
    {
        eSataController = 0x06,
        eNVMeController = 0x08,
    };
    enum class ProgIf : u8
    {
        eAHCIv1     = 0x01,
        eNVMeXpress = 0x02,
    };
    enum class HeaderType : u8
    {
        eDevice        = 0x00,
        ePciBridge     = 0x01,
        eCardBusBridge = 0x02,
    };

    struct [[gnu::packed]] DeviceHeader
    {
        u16        DeviceID;
        VendorID   VendorID;
        Status     Status;
        Command    Command;
        ClassID    ClassID;
        SubClassID SubClassID;
        ProgIf     ProgIf;
        u8         RevisionID;
        u8         BIST;
        HeaderType HeaderType;
        u8         LatencyTimer;
        u8         CacheLineSize;
    };
}; // namespace PCI
