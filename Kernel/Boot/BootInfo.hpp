/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootModuleInfo.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Time.hpp>

#include <limine.h>

constexpr u32 FRAMEBUFFER_MEMORY_MODEL_RGB = LIMINE_FRAMEBUFFER_RGB;

using Framebuffer                          = limine_framebuffer;
enum class FirmwareType : i32
{
    eUndefined = -1,
    eX86Bios   = 0,
    eUefi32    = 1,
    eUefi64    = 2,
    eSbi       = 3,
};
struct EfiMemoryMap
{
    Pointer Address           = nullptr;
    usize   Size              = 0;
    usize   DescriptorSize    = 0;
    u32     DescriptorVersion = 0;
};

enum class BootPagingMode
{
    eNone   = 0,
    eLevel4 = 4,
    eLevel5 = 5,
};
struct BootMemoryInfo
{
    Pointer             KernelPhysicalBase = nullptr;
    Pointer             KernelVirtualBase  = nullptr;
    usize               HigherHalfOffset   = 0;
    BootPagingMode      PagingMode         = BootPagingMode::eNone;
    MemoryMap           MemoryMap          = {nullptr, 0};
    struct EfiMemoryMap EfiMemoryMap;
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
    Pointer                             RSDP           = nullptr;
    // Physical address of the Device Tree Blob
    Pointer                             DeviceTreeBlob = nullptr;
    // Physical address of the SMBIOS 32 bit EntryPoint structure
    Pointer                             SmBios32Phys   = nullptr;
    // Physical address of the SMBIOS 64 bit EntryPoint structure
    Pointer                             SmBios64Phys   = nullptr;

    Pointer                             EfiSystemTable = nullptr;
};

namespace BootInfo
{
    u64                 GetHHDMOffset();
    Framebuffer**       GetFramebuffers(usize& outCount);
    Framebuffer*        GetPrimaryFramebuffer();
    usize               PagingMode();
    limine_mp_response* SMP_Response();
}; // namespace BootInfo
