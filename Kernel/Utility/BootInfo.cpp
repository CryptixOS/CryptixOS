/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "BootInfo.hpp"

#include "Common.hpp"

#include <string.h>

#define LIMINE_REQUEST                                                         \
    __attribute__((used, section(".limine_requests"))) volatile

namespace
{
    __attribute__((
        used, section(".limine_requests"))) volatile LIMINE_BASE_REVISION(3);
}

static constexpr const u32 DEFAULT_STACK_SIZE = 65536;

namespace BootInfo
{
    extern "C" void Initialize();
}

LIMINE_REQUEST limine_bootloader_info_request bootloaderInfoRequest = {
    .id       = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_stack_size_request stackSizeRequest = {
    .id         = LIMINE_STACK_SIZE_REQUEST,
    .revision   = 0,
    .response   = nullptr,
    .stack_size = DEFAULT_STACK_SIZE,
};
LIMINE_REQUEST limine_hhdm_request hhdmRequest = {
    .id       = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_framebuffer_request framebufferRequest = {
    .id       = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_paging_mode_request pagingModeRequest = {
    .id       = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .mode     = LIMINE_PAGING_MODE_DEFAULT,
    .max_mode = LIMINE_PAGING_MODE_DEFAULT,
    .min_mode = LIMINE_PAGING_MODE_DEFAULT,
};
LIMINE_REQUEST limine_smp_request smpRequest = {
    .id       = LIMINE_SMP_REQUEST,
    .revision = 0,
    .response = nullptr,
    .flags    = 0,
};
LIMINE_REQUEST limine_memmap_request memmapRequest = {
    .id       = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_entry_point_request entryPointRequest = {
    .id       = LIMINE_ENTRY_POINT_REQUEST,
    .revision = 0,
    .response = nullptr,
    .entry    = BootInfo::Initialize,
};
LIMINE_REQUEST limine_kernel_file_request kernelFileRequest = {
    .id       = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_module_request moduleRequest = {
    .id                    = LIMINE_MODULE_REQUEST,
    .revision              = 0,
    .response              = nullptr,

    .internal_module_count = 0,
    .internal_modules      = nullptr,
};
LIMINE_REQUEST limine_rsdp_request rsdpRequest = {
    .id       = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_smbios_request smbiosRequest = {
    .id       = LIMINE_SMBIOS_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_efi_system_table_request efiSystemTableRequest = {
    .id       = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_boot_time_request bootTimeRequest = {
    .id       = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_kernel_address_request kernelAddressRequest = {
    .id       = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_dtb_request dtbRequest = {
    .id       = LIMINE_DTB_REQUEST,
    .revision = 0,
    .response = nullptr,
};

namespace
{

    __attribute__((
        used,
        section(
            ".limine_requests_start"))) volatile LIMINE_REQUESTS_START_MARKER;

    __attribute__((
        used,
        section(".limine_requests_end"))) volatile LIMINE_REQUESTS_END_MARKER;

} // namespace

namespace
{
    MemoryMapEntry** memoryMap           = nullptr;
    u64              memoryMapEntryCount = 0;
} // namespace

extern "C" [[noreturn]]
void kernelStart();

namespace BootInfo
{
    extern "C" void Initialize()
    {
        (void)stackSizeRequest.response;
        (void)entryPointRequest.response;

        __asm__ volatile("outb %0, %1" : : "a"('L'), "d"(0xe9));
        // Logger::EnableE9Logging();
        if (LIMINE_BASE_REVISION_SUPPORTED == false)
            Panic("Boot: Limine base revision is not supported");
        if (!framebufferRequest.response
            || framebufferRequest.response->framebuffer_count < 1)
            Panic("Boot: Failed to acquire the framebuffer!");
        if (!memmapRequest.response || memmapRequest.response->entry_count == 0)
            Panic("Boot: Failed to acquire limine memory map entries");

        memoryMap = reinterpret_cast<MemoryMapEntry**>(
            memmapRequest.response->entries);
        memoryMapEntryCount = memmapRequest.response->entry_count;

        kernelStart();
    }
    const char* GetBootloaderName()
    {
        return bootloaderInfoRequest.response->name;
    }
    const char* GetBootloaderVersion()
    {
        return bootloaderInfoRequest.response->version;
    }
    u64          GetHHDMOffset() { return hhdmRequest.response->offset; }
    Framebuffer* GetFramebuffer()
    {
        return framebufferRequest.response->framebuffers[0];
    }
    limine_smp_response* GetSMP_Response() { return smpRequest.response; }
    MemoryMap            GetMemoryMap(u64& entryCount)
    {
        entryCount = memoryMapEntryCount;
        return memoryMap;
    }
    limine_file* FindModule(const char* name)
    {
        for (usize i = 0; i < moduleRequest.response->module_count; i++)
        {
            if (!strcmp(moduleRequest.response->modules[i]->cmdline, name))
                return moduleRequest.response->modules[i];
        }
        return nullptr;
    }
    uintptr_t GetRSDPAddress()

    {
        return reinterpret_cast<uintptr_t>(rsdpRequest.response->address);
    }
    u64 GetBootTime() { return bootTimeRequest.response->boot_time; }
    u64 GetKernelPhysicalAddress()
    {
        return kernelAddressRequest.response->physical_base;
    }
    u64 GetKernelVirtualAddress()
    {
        return kernelAddressRequest.response->virtual_base;
    }

    usize        GetPagingMode() { return pagingModeRequest.response->mode; }
    limine_file* GetKernelFile()
    {
        return kernelFileRequest.response->kernel_file;
    }

} // namespace BootInfo
