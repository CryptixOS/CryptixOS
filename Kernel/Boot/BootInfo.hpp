/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootMemoryInfo.hpp>
#include <Boot/BootModuleInfo.hpp>
#include <Drivers/Video/Framebuffer.hpp>

#include <Library/Color.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Time.hpp>

#include <limine.h>

enum class FirmwareType : i32
{
    eUndefined = -1,
    eX86Bios   = 0,
    eUefi32    = 1,
    eUefi64    = 2,
    eSbi       = 3,
};

struct BootInformation
{
    StringView                          BootloaderName    = ""_sv;
    StringView                          BootloaderVersion = ""_sv;
    StringView                          KernelCommandLine = ""_sv;
    enum FirmwareType                   FirmwareType;
    DateTime                            DateAtBoot = 0;

    BootModuleInfo                      KernelExecutable{};
    Span<BootModuleInfo, DynamicExtent> KernelModules;

    u64                                 BspID          = 0;
    u64                                 ProcessorCount = 0;
    BootMemoryInfo                      MemoryInformation{};
    Span<Framebuffer, DynamicExtent>    Framebuffers;

    // Physical address of the RSDP structure
    Pointer                             RSDP                    = nullptr;
    // Physical address of the Device Tree Blob
    Pointer                             DeviceTreeBlob          = nullptr;
    // Physical address of the SMBIOS 32 bit EntryPoint structure
    Pointer                             SmBios32Phys            = nullptr;
    // Physical address of the SMBIOS 64 bit EntryPoint structure
    Pointer                             SmBios64Phys            = nullptr;

    Pointer                             EfiSystemTable          = nullptr;

    Pointer                             LowestAllocatedAddress  = 0;
    Pointer                             HighestAllocatedAddress = 0;
};
