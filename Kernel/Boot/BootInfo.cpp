/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/CPU.hpp>
#include <Boot/BootInfo.hpp>

#include <Utility/Logger.hpp>

#include <string.h>

#define LIMINE_REQUEST                                                         \
    __attribute__((used, section(".limine_requests"))) volatile

namespace
{
    __attribute__((
        used, section(".limine_requests"))) volatile LIMINE_BASE_REVISION(3);
}

namespace BootInfo
{
    extern "C" __attribute__((no_sanitize("address"))) void Initialize();
}

LIMINE_REQUEST limine_bootloader_info_request s_BootloaderInfoRequest = {
    .id       = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_firmware_type_request s_FirmwareTypeRequest = {
    .id       = LIMINE_FIRMWARE_TYPE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_stack_size_request s_StackSizeRequest = {
    .id         = LIMINE_STACK_SIZE_REQUEST,
    .revision   = 0,
    .response   = nullptr,
    .stack_size = CPU::KERNEL_STACK_SIZE,
};
LIMINE_REQUEST limine_hhdm_request s_HhdmRequest = {
    .id       = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_framebuffer_request s_FramebufferRequest = {
    .id       = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_paging_mode_request s_PagingModeRequest = {
    .id       = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .mode     = LIMINE_PAGING_MODE_DEFAULT,
    .max_mode = LIMINE_PAGING_MODE_DEFAULT,
    .min_mode = LIMINE_PAGING_MODE_DEFAULT,
};
LIMINE_REQUEST limine_mp_request s_SmpRequest = {
    .id       = LIMINE_MP_REQUEST,
    .revision = 0,
    .response = nullptr,
    .flags    = 0,
};
LIMINE_REQUEST limine_memmap_request s_MemmapRequest = {
    .id       = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_entry_point_request s_EntryPointRequest = {
    .id       = LIMINE_ENTRY_POINT_REQUEST,
    .revision = 0,
    .response = nullptr,
    .entry    = BootInfo::Initialize,
};
LIMINE_REQUEST limine_executable_file_request s_KernelFileRequest = {
    .id       = LIMINE_EXECUTABLE_FILE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_module_request s_ModuleRequest = {
    .id                    = LIMINE_MODULE_REQUEST,
    .revision              = 0,
    .response              = nullptr,

    .internal_module_count = 0,
    .internal_modules      = nullptr,
};
LIMINE_REQUEST limine_rsdp_request s_RsdpRequest = {
    .id       = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_smbios_request s_SmbiosRequest = {
    .id       = LIMINE_SMBIOS_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_efi_system_table_request s_EfiSystemTableRequest = {
    .id       = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_boot_time_request s_BootTimeRequest = {
    .id       = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_executable_address_request s_KernelAddressRequest = {
    .id       = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_dtb_request s_DtbRequest = {
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
    FirmwareType     s_FirmwareType      = FirmwareType::eUndefined;
    MemoryMapEntry** memoryMap           = nullptr;
    u64              memoryMapEntryCount = 0;
} // namespace

extern "C" CTOS_NO_KASAN [[noreturn]]
void kernelStart();

namespace BootInfo
{
    extern "C" __attribute__((no_sanitize("address"))) void Initialize()
    {
        (void)s_StackSizeRequest.response;
        (void)s_EntryPointRequest.response;

        Logger::EnableOutput(LOG_OUTPUT_E9);

        if (LIMINE_BASE_REVISION_SUPPORTED == false)
            EarlyPanic("Boot: Limine base revision is not supported");
        if (!s_FramebufferRequest.response
            || s_FramebufferRequest.response->framebuffer_count < 1)
            EarlyPanic("Boot: Failed to acquire the framebuffer!");
        Logger::EnableOutput(LOG_OUTPUT_TERMINAL);

        if (!s_MemmapRequest.response
            || s_MemmapRequest.response->entry_count == 0)
            Panic("Boot: Failed to acquire limine memory map entries");

        memoryMap = reinterpret_cast<MemoryMapEntry**>(
            s_MemmapRequest.response->entries);
        memoryMapEntryCount = s_MemmapRequest.response->entry_count;

        switch (s_FirmwareTypeRequest.response->firmware_type)
        {
            case LIMINE_FIRMWARE_TYPE_X86BIOS:
                s_FirmwareType = FirmwareType::eX86Bios;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI32:
                s_FirmwareType = FirmwareType::eUefi32;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI64:
                s_FirmwareType = FirmwareType::eUefi64;
                break;
            case LIMINE_FIRMWARE_TYPE_SBI:
                s_FirmwareType = FirmwareType::eSbi;
                break;
        }

        kernelStart();
    }
    const char* GetBootloaderName()
    {
        return s_BootloaderInfoRequest.response->name;
    }
    const char* GetBootloaderVersion()
    {
        return s_BootloaderInfoRequest.response->version;
    }
    FirmwareType GetFirmwareType() { return s_FirmwareType; }

    u64          GetHHDMOffset() { return s_HhdmRequest.response->offset; }
    Framebuffer* GetFramebuffer()
    {
        return s_FramebufferRequest.response->framebuffers[0];
    }
    Framebuffer** GetFramebuffers(usize& outCount)
    {
        outCount = s_FramebufferRequest.response->framebuffer_count;
        return s_FramebufferRequest.response->framebuffers;
    }
    limine_mp_response* GetSMP_Response() { return s_SmpRequest.response; }
    MemoryMap           GetMemoryMap(u64& entryCount)
    {
        entryCount = memoryMapEntryCount;
        return memoryMap;
    }
    limine_file* FindModule(const char* name)
    {
        for (usize i = 0; i < s_ModuleRequest.response->module_count; i++)
        {
            if (!strcmp(s_ModuleRequest.response->modules[i]->cmdline, name))
                return s_ModuleRequest.response->modules[i];
        }
        return nullptr;
    }
    Pointer GetRSDPAddress()

    {
        return reinterpret_cast<uintptr_t>(s_RsdpRequest.response->address);
    }
    std::pair<Pointer, Pointer> GetSmBiosEntries()
    {
        return std::make_pair(s_SmbiosRequest.response->entry_32,
                              s_SmbiosRequest.response->entry_64);
    }

    u64     GetBootTime() { return s_BootTimeRequest.response->boot_time; }
    Pointer GetKernelPhysicalAddress()
    {
        return s_KernelAddressRequest.response->physical_base;
    }
    Pointer GetKernelVirtualAddress()
    {
        return s_KernelAddressRequest.response->virtual_base;
    }

    usize        GetPagingMode() { return s_PagingModeRequest.response->mode; }
    limine_file* GetKernelFile()
    {
        return s_KernelFileRequest.response->executable_file;
    }

} // namespace BootInfo
