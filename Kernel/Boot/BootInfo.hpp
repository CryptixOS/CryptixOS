/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringView.hpp>

#include <limine.h>

#include <utility>

constexpr u32 FRAMEBUFFER_MEMORY_MODEL_RGB = LIMINE_FRAMEBUFFER_RGB;

constexpr u32 MEMORY_MAP_USABLE            = LIMINE_MEMMAP_USABLE;
constexpr u32 MEMORY_MAP_RESERVED          = LIMINE_MEMMAP_RESERVED;
constexpr u32 MEMORY_MAP_ACPI_RECLAIMABLE  = LIMINE_MEMMAP_ACPI_RECLAIMABLE;
constexpr u32 MEMORY_MAP_ACPI_NVS          = LIMINE_MEMMAP_ACPI_NVS;
constexpr u32 MEMORY_MAP_BAD_MEMORY        = LIMINE_MEMMAP_BAD_MEMORY;
constexpr u32 MEMORY_MAP_BOOTLOADER_RECLAIMABLE
    = LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
constexpr u32 MEMORY_MAP_KERNEL_AND_MODULES
    = LIMINE_MEMMAP_EXECUTABLE_AND_MODULES;
constexpr u32 MEMORY_MAP_FRAMEBUFFER = LIMINE_MEMMAP_FRAMEBUFFER;

using MemoryMapEntry                 = limine_memmap_entry;
using MemoryMap                      = MemoryMapEntry**;
using Framebuffer                    = limine_framebuffer;

enum class FirmwareType : i32
{
    eUndefined = -1,
    eX86Bios   = 0,
    eUefi32    = 1,
    eUefi64    = 2,
    eSbi       = 3,
};

namespace BootInfo
{
    const char*                         GetBootloaderName();
    const char*                         GetBootloaderVersion();
    StringView                          GetKernelCommandLine();
    FirmwareType                        GetFirmwareType();
    u64                                 GetHHDMOffset();
    Framebuffer**                       GetFramebuffers(usize& outCount);
    Framebuffer*                        GetPrimaryFramebuffer();
    usize                               GetPagingMode();
    limine_mp_response*                 GetSMP_Response();
    MemoryMap                           GetMemoryMap(u64& entryCount);
    limine_file*                        GetExecutableFile();
    limine_file*                        FindModule(const char* name);
    PM::Pointer                         GetRSDPAddress();
    std::pair<PM::Pointer, PM::Pointer> GetSmBiosEntries();
    Pointer                             GetEfiSystemTable();
    limine_efi_memmap_response*         GetEfiMemoryMap();
    u64                                 GetDateAtBoot();
    Pointer                             GetKernelPhysicalAddress();
    Pointer                             GetKernelVirtualAddress();
    Pointer                             GetDeviceTreeBlobAddress();
}; // namespace BootInfo
