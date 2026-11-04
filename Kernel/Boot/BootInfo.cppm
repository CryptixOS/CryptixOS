/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Core/UUID.hpp>
#include <Prism/Network/IPv4Address.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Time.hpp>
#include <limine.h>

export module Cryptix:Boot.BootInfo;

export import :Boot.BootMemoryInfo;
export import :Boot.BootModuleInfo;
export import :Drivers.Video.Framebuffer;
export import :Library.Color;
export import :Memory.PMM;

export enum class FirmwareType : i32
{
    eUndefined = -1,
    eX86Bios   = 0,
    eUefi32    = 1,
    eUefi64    = 2,
    eSbi       = 3,
};

export struct BootInformation
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
