/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/EFI/Types.hpp>

namespace EFI
{
    struct Time
    {
        Uint16 Year;
        Uint8  Month;
        Uint8  Day;
        Uint8  Hour;
        Uint8  Minute;
        Uint8  Second;
        Uint8  Pad1;
        Uint32 Nanosecond;
        Int16  TimeZone;
        Uint8  Daylight;
        Uint8  Pad2;
    };
    constexpr usize AdjustDaylight      = 0x01;
    constexpr usize InDaylight          = 0x02;
    constexpr usize UnspecifiedTimezone = 0x07ff;

    struct TimeCapabilities
    {
        Uint32  Resolution;
        Uint32  Accuracy;
        Boolean SetsToZero;
    };

    enum class ResetType
    {
        eCold,
        eWarm,
        eShutdown,
        ePlatformSpecific,
    };

    // TODO(v1tr10l7): ->
    using PhysicalAddress = Size;
    struct CapsuleBlockDescriptor
    {
        Uint64 Length;
        union
        {
            PhysicalAddress DataBlock;
            PhysicalAddress ContinuationPointer;
        };
    };
    struct CapsuleHeader
    {
        Guid   CapsuleGuid;
        Uint32 HeaderSize;
        Uint32 Flags;
        Uint32 CapsuleImageSize;
    };
    struct CapsuleTable
    {
        Uint32 CapsuleArrayNumber;
        Void*  CapsulePtr[1];
    };
    struct MemoryRange
    {
        PhysicalAddress Address;
        Uint64          Length;
    };
    struct MemoryRangeCapsule
    {
        CapsuleHeader Header;
        Uint32        OsRequestedMemoryType;
        Uint64        NumberOfMemoryRanges;
        MemoryRange   MemoryRanges[];
    };
    struct MemoryRangeCapsuleResult
    {
        Uint64 FirmwareMemoryRequirement;
        Uint64 NumberOfMemoryRanges;
    };

    using MemoryDescriptor = void;

    using GetTimeFn = Status (*)(Time* time, TimeCapabilities* capabilities);
    using SetTimeFn = Status (*)(Time* time);
    using GetWakeUpTimeFn
        = Status (*)(bool* enabled, bool* pending, Time* time);
    using SetWakeUpTimeFn = Status (*)(bool enable, Time* time);

    using SetVirtualAddressMapFn
        = Status (*)(usize memoryMapSize, usize descriptorSize,
                     u32 descriptorVersion, MemoryDescriptor* VirtualMap);
    using ConvertPointerFn = Status (*)(usize debugDisposition, void** address);

    using GetVariableFn
        = Status (*)(char16_t name, Guid* vendorGuid, u32* attributes,
                     usize* dataSize, void* data);
    using GetNextVariableNameFn
        = Status (*)(usize* variableNameSize, char16_t* variableName,
                     Guid* vendorGuid);

    using SetVariableFn
        = Status (*)(char16_t* variableName, Guid* vendorGuid, u32 attributes,
                     usize dataSize, void* data);

    using GetNextHighMonotonicCountFn = Status (*)(Uint32* HighCount);

    using ResetSystemFn
        = EFIAPI Status (*)(ResetType ResetType, Status ResetStatus,
                            Size DataSize, Char16* ResetData);

    using UpdateCapsuleFn
        = Status (*)(CapsuleHeader** CapsuleHeaderArray, Size CapsuleCount,
                     PhysicalAddress ScatterGather);
    using QueryCapsuleCapabilitiesFn
        = Status (*)(CapsuleHeader** CapsuleHeaderArray, Size CapsuleCount,
                     Uint64* MaximumCapsuleSize, ResetType* ResetType);

    using QueryVariableInfoFn
        = Status (*)(u32 attributes, u64* maximumVariableStorageSize,
                     u64* remainingVariableStorageSize,
                     u64* maximumVariableSize);

    struct RuntimeServices
    {
        TableHeader                 Header;

        //
        // Time Services
        //
        GetTimeFn                   GetTime;
        SetTimeFn                   SetTime;
        GetWakeUpTimeFn             GetWakeupTime;
        SetWakeUpTimeFn             SetWakeupTime;

        //
        // Virtual Memory Services
        //
        SetVirtualAddressMapFn      SetVirtualAddressMap;
        ConvertPointerFn            ConvertPointer;

        //
        // Variable Services
        //
        GetVariableFn               GetVariable;
        GetNextVariableNameFn       GetNextVariableName;
        SetVariableFn               SetVariable;

        //
        // Miscellaneous Services
        //
        GetNextHighMonotonicCountFn GetNextHighMonotonicCount;
        ResetSystemFn               ResetSystem;

        //
        // UEFI 2.0 Capsule Services
        //
        UpdateCapsuleFn             UpdateCapsule;
        QueryCapsuleCapabilitiesFn  QueryCapsuleCapabilities;

        //
        // Miscellaneous UEFI 2.0 Service
        //
        QueryVariableInfoFn         QueryVariableInfo;
    };
}; // namespace EFI
