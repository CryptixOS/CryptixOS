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
    using Status           = u64;
    using Guid             = void;
    using Time             = void;
    using TimeCapabilities = void;
    using MemoryDescriptor = void;

    enum ResetType
    {
        eResetCold,
        eResetWarm,
        eResetShutdown,
        eResetPlatformSpecific,
    };

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

    using GetNextHighMonoCountFn = void*;
    using ResetSystemFn
        = u64(__attribute__((ms_abi)) *)(ResetType resetType,
                                         Status resetStatus, usize dataSize,
                                         u16* resetData);

    using UpdateCapsuleFn            = void*;
    using QueryCapsuleCapabilitiesFn = void*;

    using QueryVariableInfoFn
        = Status (*)(u32 attributes, u64* maximumVariableStorageSize,
                     u64* remainingVariableStorageSize,
                     u64* maximumVariableSize);

    struct RuntimeServices
    {
        TableHeader                Header;

        //
        // Time Services
        //
        GetTimeFn                  GetTime;
        SetTimeFn                  SetTime;
        GetWakeUpTimeFn            GetWakeupTime;
        SetWakeUpTimeFn            SetWakeupTime;

        //
        // Virtual Memory Services
        //
        SetVirtualAddressMapFn     SetVirtualAddressMap;
        ConvertPointerFn           ConvertPointer;

        //
        // Variable Services
        //
        GetVariableFn              GetVariable;
        GetNextVariableNameFn      GetNextVariableName;
        SetVariableFn              SetVariable;

        //
        // Miscellaneous Services
        //
        GetNextHighMonoCountFn     GetNextHighMonotonicCount;
        ResetSystemFn              ResetSystem;

        //
        // UEFI 2.0 Capsule Services
        //
        UpdateCapsuleFn            UpdateCapsule;
        QueryCapsuleCapabilitiesFn QueryCapsuleCapabilities;

        //
        // Miscellaneous UEFI 2.0 Service
        //
        QueryVariableInfoFn        QueryVariableInfo;
    };
}; // namespace EFI
