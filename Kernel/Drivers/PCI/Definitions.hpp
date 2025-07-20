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
        eCapabilityList        = Bit(4),
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
    constexpr bool operator&(const Status lhs, const Status rhs)
    {
        return ToUnderlying(lhs) & ToUnderlying(rhs);
    }

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
        u16             DeviceID;
        enum VendorID   VendorID;
        enum Status     Status;
        enum Command    Command;
        enum ClassID    ClassID;
        enum SubClassID SubClassID;
        enum ProgIf     ProgIf;
        u8              RevisionID;
        u8              BIST;
        enum HeaderType HeaderType;
        u8              LatencyTimer;
        u8              CacheLineSize;
    };

    enum class CapabilityID : u8
    {
        eNull                          = 0x00,
        ePowerManagement               = 0x01,
        // Accelerated Graphics Port
        eAGP                           = 0x02,
        // Vital Product Data
        eVPD                           = 0x03,
        eSlotID                        = 0x04,
        // Message Signaled Interrupts
        eMSI                           = 0x05,
        // Compact PCI Hot Swap
        eCompactHotSwap                = 0x06,
        ePCIx                          = 0x07,
        eHyperTransport                = 0x08,
        eVendorSpecific                = 0x09,
        eDebug                         = 0x0a,
        eCompactCentralResourceControl = 0x0b,
        eHotPlug                       = 0x0c,
        eBridgeSubsystemVendorID       = 0x0d,
        eAGP8x                         = 0x0e,
        eSecureDevice                  = 0x0f,
        ePCIe                          = 0x10,
        eMSIx                          = 0x11,
        eSATAConfiguration             = 0x12,
        eAdvancedFeatures              = 0x13,
        eEnhancedAllocation            = 0x14,
        eFlatteningPortalBridge        = 0x15,
    };
    enum class ExtendedCapabilityID : u8
    {
        eNull                                         = 0x00,
        eAdvancedErrorReporting                       = 0x01,
        eVirtualChannel                               = 0x02,
        eDeviceSerialNumber                           = 0x03,
        ePowerBudgeting                               = 0x04,
        eRootComplexLinkDeclaration                   = 0x05,
        eRootComplexInternalLinkControl               = 0x06,
        eRootComplexEventCollectorEndPointAssociation = 0x07,
        eMultiFunctionVirtualChannel                  = 0x08,
        eVirtualChannelMFVCPresent                    = 0x09,
        eRootComplexRegisterBlock                     = 0x0a,
        eVendorSpecific                               = 0x0b,
        eConfigurationAccessCorrelation               = 0x0c,
        eAccessControlServices                        = 0x0d,
        eAlternateRoutingIdInterpretation             = 0x0e,
        eAddressTranslationServices                   = 0x0f,
        eSingleRootIoVirtualization                   = 0x10,
        eMultiRootIoVirtualization                    = 0x11,
        eMulticast                                    = 0x12,
        ePageRequestInterface                         = 0x13,
        eReservedForAmd                               = 0x14,
        eResizableBar                                 = 0x15,
        eDynamicPowerAllocation                       = 0x16,
        eTphRequester                                 = 0x17,
        eLatencyToleranceReporting                    = 0x18,
        eSecondaryPCIe                                = 0x19,
        eProtocolMultiplexing                         = 0x1a,
        eProcessAddressSpaceID                        = 0x1b,
        eLnRequester                                  = 0x1c,
        eDownstreamPortContainment                    = 0x1d,
        eL1PmSubstates                                = 0x1e,
        ePrecisionTimeMeasurement                     = 0x1f,
        ePCIeM                                        = 0x20,
        eFrsQueueing                                  = 0x21,
        eReadinessTimeReporting                       = 0x22,
        eDesignatedVendor                             = 0x23,
        eVfResizableBar                               = 0x24,
        eDataLinkFeature                              = 0x25,
        ePhysicalLayer16GTps                          = 0x26,
        eLaneMargining                                = 0x27,
        eHierarchyID                                  = 0x28,
        eNativePCIeEnclosureManagement                = 0x29,
        ePhysicalLayer32GTps                          = 0x2a,
        eAlternateProtocol                            = 0x2b,
        eSystemFirmwareIntermediary                   = 0x2c,
    };

    enum class PowerManagementCapability
    {

    };
}; // namespace PCI
