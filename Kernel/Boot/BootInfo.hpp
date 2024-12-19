#pragma once

#include "Utility/Types.hpp"

#define LIMINE_API_REVISION 2
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
    const char*                 GetBootloaderName();
    const char*                 GetBootloaderVersion();
    FirmwareType                GetFirmwareType();
    u64                         GetHHDMOffset();
    Framebuffer*                GetFramebuffer();
    Framebuffer**               GetFramebuffers(usize& outCount);
    limine_mp_response*         GetSMP_Response();
    MemoryMap                   GetMemoryMap(u64& entryCount);
    limine_file*                FindModule(const char* name);
    Pointer                     GetRSDPAddress();
    std::pair<Pointer, Pointer> GetSmBiosEntries();
    u64                         GetBootTime();
    Pointer                     GetKernelPhysicalAddress();
    Pointer                     GetKernelVirtualAddress();
    usize                       GetPagingMode();
    limine_file*                GetKernelFile();
}; // namespace BootInfo
