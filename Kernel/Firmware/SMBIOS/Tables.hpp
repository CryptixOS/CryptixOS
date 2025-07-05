/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace DMI
{
    // Common header for all  structures
    struct Header
    {
        u8  Type;
        u8  Length;
        u16 Handle;
    };

    // Type 0 — BIOS Information
    struct [[gnu::packed]] FirmwareInformation : Header
    {
        u8  Vendor;
        u8  Version;
        u16 BiosStartSegment;
        u8  ReleaseDate;
        u8  RomSize;
        u64 Characteristics;
        u16 Reserved;
        u8  MajorRelease;
        u8  MinorRelease;
        u8  EmbeddedControllerMajor;
        u8  EmbeddedControllerMinor;
        u16 ExtendedRomSize;
    };

    // Type 1 — System Information
    struct [[gnu::packed]] SystemInformation : Header
    {
        u8  Name;
        u8  Version;
        u8  SerialNumber;
        u8  UUID[16];
        u8  WakeUpType;
        u8  SkuNumber;
        u8  Family;
    };
    struct [[gnu::packed]] SystemInfo : Header
    {
        u8 Manufacturer;
        u8 ProductName;
        u8 Version;
        u8 SerialNumber;
        u8 Uuid[16];
        u8 WakeUpType;
        u8 SkuNumber;
        u8 Family;
    };

    // Type 2 — Baseboard (Module) Information
    struct [[gnu::packed]] BaseboardInfo : Header
    {
        u8  Manufacturer;
        u8  Product;
        u8  Version;
        u8  SerialNumber;
        u8  AssetTag;
        u8  FeatureFlags;
        u8  LocationInChassis;
        u16 ChassisHandle;
        u8  BoardType;
        u8  NumberOfContainedObjectHandles;
        // followed by contained object handles[]
    };

    // Type 3 — System Enclosure (Chassis)
    struct [[gnu::packed]] ChassisInfo : Header
    {
        u8  Manufacturer;
        u8  Type;
        u8  Version;
        u8  SerialNumber;
        u8  AssetTag;
        u8  BootUpState;
        u8  PowerSupplyState;
        u8  ThermalState;
        u8  SecurityStatus;
        u32 OemDefined;
        u8  Height;
        u8  NumberOfPowerCords;
        u8  ContainedElementCount;
        u8  ContainedElementRecordLength;
        // followed by contained elements...
    };

    // Type 4 — Processor Information
    struct [[gnu::packed]] ProcessorInfo : Header
    {
        u8  SocketDesignation;
        u8  ProcessorType;
        u8  ProcessorFamily;
        u8  ProcessorManufacturer;
        u64 ProcessorID;
        u8  ProcessorVersion;
        u8  Voltage;
        u16 ExternalClock;
        u16 MaxSpeed;
        u16 CurrentSpeed;
        u8  Status;
        u8  ProcessorUpgrade;
        u16 L1CacheHandle;
        u16 L2CacheHandle;
        u16 L3CacheHandle;
        u8  SerialNumber;
        u8  AssetTag;
        u8  PartNumber;
        u8  CoreCount;
        u8  CoreEnabled;
        u8  ThreadCount;
    };

    // Type 7 — Cache Information
    struct [[gnu::packed]] CacheInfo : Header
    {
        u8  CacheSocketDesignation;
        u16 CacheConfiguration;
        u16 MaximumCacheSize;
        u16 InstalledSize;
        u8  SupportedSRAMType;
        u8  CurrentSRAMType;
        u8  CacheSpeed;
        u8  ErrorCorrectionType;
        u8  SystemCacheType;
        u8  Associativity;
    };

    // Type 8 — Port Connector Information
    struct [[gnu::packed]] Port_connectorInfo : Header
    {
        u8 InternalReferenceDesignation;
        u8 InternalConnectorType;
        u8 ExternalReferenceDesignation;
        u8 ExternalConnectorType;
        u8 PortType;
    };

    // Type 9 — System Slots
    struct [[gnu::packed]] SystemSlot : Header
    {
        u8  SlotDesignation;
        u8  SlotType;
        u8  SlotDataBusWidth;
        u8  CurrentUsage;
        u8  SlotLength;
        u16 SlotID;
        u8  SlotCharacteristics1;
        u16 SegmentGroupNumber;
        u16 BusNumber;
        u8  DeviceFunctionNumber;
        u8  SlotCharacteristics2;
    };

    // Type 11 — OEM Strings
    struct [[gnu::packed]] OemStrings : Header
    {
        u8 Count;
        // followed by count string indices
    };

    // Type 12 — System Configuration Options
    struct [[gnu::packed]] System_configOptions : Header
    {
        u8 Count;
        // followed by string indices
    };

    // Type 13 — BIOS Language Information
    struct [[gnu::packed]] BiosLanguage : Header
    {
        u8 InstallableLanguages;
        u8 Flags;
        // followed by string indices and optionally multi-language codes
    };

    // Type 14 — Group Associations
    struct [[gnu::packed]] GroupAssociations : Header
    {
        u8 GroupName;
        u8 GroupKey;
        u8 ItemCount;
        // followed by item handles[]
    };

    // Type 15 — System Event Log
    struct [[gnu::packed]] System_eventLog : Header
    {
        u16 LogAreaLength;
        u32 LogHeaderStartOffset;
        u32 LogDataStartOffset;
        u8  AccessMethod;
        u16 LogStatus;
        u16 LogChangeToken;
        u8  AccessMethodAddress;
        // followed by header/retry string section
    };

    // Type 16 — Physical Memory Array
    struct [[gnu::packed]] Physical_memoryArray : Header
    {
        u8  Location;
        u8  Use;
        u8  MemoryErrorCorrection;
        u32 MaximumCapacity;
        u16 MemoryErrorInfoHandle;
        u8  NumberOfMemoryDevices;
    };

    // Type 17 — Memory Device
    struct [[gnu::packed]] MemoryDevice : Header
    {
        u16 ArrayHandle;
        u16 MemoryErrorInfoHandle;
        u16 TotalWidth;
        u16 DataWidth;
        u16 Size; // in MB (0x7FFF = unknown, 0xFFFF = >32GB)
        u8  FormFactor;
        u8  DeviceSet;
        u8  DeviceLocator;
        u8  BankLocator;
        u8  MemoryType;
        u16 TypeDetail;
        u16 Speed;
        u8  Manufacturer;
        u8  SerialNumber;
        u8  AssetTag;
        u8  PartNumber;
        u8  Attributes;
        // optional extended size etc.
    };

    // Type 18 — 32-Bit Memory Error Information
    struct [[gnu::packed]] Memory_error32 : Header
    {
        u16 ErrorType;
        u16 ErrorGranularity;
        u16 ErrorOperation;
        u32 VendorSyndrome;
        u32 MemoryArrayAddress;
        u32 DeviceAddress;
        u32 Resolution;
    };

    // Type 19 — Memory Array Mapped Address
    struct [[gnu::packed]] Memory_array_mappedAddress : Header
    {
        u32 StartingAddress;
        u32 EndingAddress;
        u16 MemoryArrayHandle;
        u8  PartitionWidth;
    };

    // Type 20 — Memory Device Mapped Address
    struct [[gnu::packed]] Memory_device_mappedAddress : Header
    {
        u32 StartingAddress;
        u32 EndingAddress;
        u16 MemoryDeviceHandle;
        u16 MemoryArrayMappedAddressHandle;
        u8  PartitionRowPosition;
        u8  InterleavePosition;
        u8  InterleavedDataDepth;
        // optional extended...
    };

    // Type 21 — Built-in Pointing Device
    struct [[gnu::packed]] PointingDevice : Header
    {
        u8 Type;
        u8 Interface;
        u8 Buttons;
    };

    // Type 22 — Portable Battery
    struct [[gnu::packed]] PortableBattery : Header
    {
        u8  Location;
        u8  Manufacturer;
        u8  ManufactureDate;
        u8  SerialNumber;
        u8  DeviceName;
        u8  DesignCapacity;
        u8  DesignVoltage;
        u8  SbdsVersion;
        u16 MaximumError;
        u8  SbdsSerialNumber;
        u16 SbdsManufactureDate;
        u16 SbdsChemistry;
        u16 DesignCapacityMultiplier;
        u8  OemSpecific;
    };

    // Type 23 — System Reset
    struct [[gnu::packed]] SystemReset : Header
    {
        u8 Capabilities;
    };

    // ... Types 24 to 43 included similarly ...

    // Type 127 — End-of-Table
    struct EndOfTable : Header
    {
    };
}; // namespace DMI
