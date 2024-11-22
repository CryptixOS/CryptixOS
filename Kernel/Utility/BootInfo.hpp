#pragma once

#include "Utility/Types.hpp"

#include <limine.h>

static constexpr const u32 FRAMEBUFFER_MEMORY_MODEL_RGB
    = LIMINE_FRAMEBUFFER_RGB;

inline static constexpr const u32 MEMORY_MAP_USABLE   = LIMINE_MEMMAP_USABLE;
inline static constexpr const u32 MEMORY_MAP_RESERVED = LIMINE_MEMMAP_RESERVED;
inline static constexpr const u32 MEMORY_MAP_ACPI_RECLAIMABLE
    = LIMINE_MEMMAP_ACPI_RECLAIMABLE;
inline static constexpr const u32 MEMORY_MAP_ACPI_NVS = LIMINE_MEMMAP_ACPI_NVS;
inline static constexpr const u32 MEMORY_MAP_BAD_MEMORY
    = LIMINE_MEMMAP_BAD_MEMORY;
inline static constexpr const u32 MEMORY_MAP_BOOTLOADER_RECLAIMABLE
    = LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
inline static constexpr const u32 MEMORY_MAP_KERNEL_AND_MODULES
    = LIMINE_MEMMAP_KERNEL_AND_MODULES;
inline static constexpr const u32 MEMORY_MAP_FRAMEBUFFER
    = LIMINE_MEMMAP_FRAMEBUFFER;

using MemoryMapEntry = limine_memmap_entry;
using MemoryMap      = MemoryMapEntry**;
using Framebuffer    = limine_framebuffer;

namespace BootInfo
{
    const char*          GetBootloaderName();
    const char*          GetBootloaderVersion();
    u64                  GetHHDMOffset();
    Framebuffer*         GetFramebuffer();
    limine_smp_response* GetSMP_Response();
    MemoryMap            GetMemoryMap(u64& entryCount);
    limine_file*         FindModule(const char* name);
    uintptr_t            GetRSDPAddress();
    u64                  GetBootTime();
    u64                  GetKernelPhysicalAddress();
    u64                  GetKernelVirtualAddress();
    usize                GetPagingMode();
    limine_file*         GetKernelFile();
}; // namespace BootInfo
