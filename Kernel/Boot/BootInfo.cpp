/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/CPU.hpp>
#include <Boot/BootInfo.hpp>

#include <Library/Logger.hpp>

#include <cstring>

#define LIMINE_REQUEST                                                         \
    __attribute__((used, section(".limine_requests"))) volatile

namespace
{
    __attribute__((
        used, section(".limine_requests"))) volatile LIMINE_BASE_REVISION(2);
} // namespace

namespace BootInfo
{
    extern "C" __attribute__((no_sanitize("address"))) void Initialize();
}

#pragma region limine requests
LIMINE_REQUEST limine_bootloader_info_request s_BootloaderInfoRequest = {
    .id       = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_executable_cmdline_request s_ExecutableCmdlineRequest = {
    .id       = LIMINE_EXECUTABLE_CMDLINE_REQUEST,
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
LIMINE_REQUEST limine_executable_file_request s_ExecutableRequest = {
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
LIMINE_REQUEST limine_efi_memmap_request s_EfiMemmapRequest = {
    .id       = LIMINE_EFI_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
LIMINE_REQUEST limine_date_at_boot_request s_DateAtBootRequest = {
    .id       = LIMINE_DATE_AT_BOOT_REQUEST,
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
#pragma endregion

namespace
{

    __attribute__((
        used,
        section(
            ".limine_requests_start"))) volatile LIMINE_REQUESTS_START_MARKER;

    __attribute__((
        used,
        section(".limine_requests_end"))) volatile LIMINE_REQUESTS_END_MARKER;

    FirmwareType s_FirmwareType = FirmwareType::eUndefined;
    MemoryMap    s_MemoryMap    = {};

    void         ParseMemoryMap()
    {
        if (!s_MemmapRequest.response
            || s_MemmapRequest.response->entry_count == 0)
            Panic("Boot: Failed to acquire limine memory map entries");

        auto** memoryMap        = s_MemmapRequest.response->entries;
        usize  memoryEntryCount = s_MemmapRequest.response->entry_count;
        auto   allocateMemory   = [&](usize bytes) -> Pointer
        {
            for (usize i = 0; i < memoryEntryCount; i++)
            {
                bytes       = Math::AlignUp(bytes, PMM::PAGE_SIZE);
                auto* entry = memoryMap[i];
                if (entry->type != LIMINE_MEMMAP_USABLE
                    || entry->length < bytes)
                    continue;

                Pointer address = entry->base;
                entry->base += bytes;
                entry->length -= bytes;

                return address.ToHigherHalf();
            }

            return nullptr;
        };
        auto fromLimineMemoryType = [](u8 type) -> MemoryType
        {
            switch (type)
            {
                case LIMINE_MEMMAP_USABLE: return MemoryType::eUsable;
                case LIMINE_MEMMAP_RESERVED: return MemoryType::eReserved;
                case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                    return MemoryType::eACPI_Reclaimable;
                case LIMINE_MEMMAP_ACPI_NVS: return MemoryType::eACPI_NVS;
                case LIMINE_MEMMAP_BAD_MEMORY: return MemoryType::eBadMemory;
                case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                    return MemoryType::eBootloaderReclaimable;
                case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
                    return MemoryType::eKernelAndModules;
                case LIMINE_MEMMAP_FRAMEBUFFER: return MemoryType::eFramebuffer;

                default: break;
            }

            return MemoryType::eReserved;
        };

        s_MemoryMap.Entries = allocateMemory(
            sizeof(MemoryRegion) * s_MemmapRequest.response->entry_count);
        s_MemoryMap.EntryCount = s_MemmapRequest.response->entry_count;

        for (usize i = 0; i < memoryEntryCount; i++)
        {
            auto*      current     = memoryMap[i];
            u64        base        = current->base;
            u64        length      = current->length;
            MemoryType type        = fromLimineMemoryType(current->type);

            s_MemoryMap.Entries[i] = MemoryRegion(base, length, type);
        }
    }
} // namespace

extern "C" CTOS_NO_KASAN [[noreturn]] void kernelStart(const BootInformation&);

#define VerifyExistenceOrRet(requestName)                                      \
    if (!requestName.response) return nullptr;
#define VerifyExistenceOrRetValue(requestName, value)                          \
    if (!requestName.response) return value;

namespace PhysicalMemoryManager
{
    void EarlyInitialize(const MemoryMap&);
};
namespace BootInfo
{
    extern "C" __attribute__((no_sanitize("address"))) void Initialize()
    {
        CtosUnused(s_StackSizeRequest.response);
        CtosUnused(s_EntryPointRequest.response);

        Logger::EnableSink(LOG_SINK_E9);
#if defined(CTOS_TARGET_X86_64) && defined(__aarch64__)
    #error "target is aarch64 and CTOS_TARGET_X86_64 is defined"
#elif defined(__aarch64__) && !defined(CTOS_TARGET_AARCH64)
    #error "target is aarch64 and CTOS_TARGET_AARCH64 is not defined"
#endif

#if defined(CTOS_TARGET_X86_64) && !defined(CTOS_TARGET_AARCH64)
        Logger::EnableSink(LOG_SINK_SERIAL);
#endif

        if (!LIMINE_BASE_REVISION_SUPPORTED)
            EarlyPanic("Boot: Limine base revision is not supported");
        if (!s_FramebufferRequest.response
            || s_FramebufferRequest.response->framebuffer_count < 1)
            EarlyPanic("Boot: Failed to acquire the framebuffer!");
        Logger::EnableSink(LOG_SINK_TERMINAL);

        BootInformation info;
        ParseMemoryMap();
        using namespace PhysicalMemoryManager;
        EarlyInitialize(s_MemoryMap);

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

        info.BootloaderName    = s_BootloaderInfoRequest.response->name;
        info.BootloaderVersion = s_BootloaderInfoRequest.response->version;
        info.KernelCommandLine = s_ExecutableCmdlineRequest.response->cmdline;
        switch (s_FirmwareTypeRequest.response->firmware_type)
        {
            case LIMINE_FIRMWARE_TYPE_X86BIOS:
                info.FirmwareType = FirmwareType::eX86Bios;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI32:
                info.FirmwareType = FirmwareType::eUefi32;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI64:
                info.FirmwareType = FirmwareType::eUefi64;
                break;
            case LIMINE_FIRMWARE_TYPE_SBI:
                info.FirmwareType = FirmwareType::eSbi;
                break;

            default: info.FirmwareType = FirmwareType::eUndefined; break;
        }
        info.DateAtBoot        = s_DateAtBootRequest.response->timestamp;

        auto  kernelFile       = s_ExecutableRequest.response->executable_file;
        auto& kernelModuleInfo = info.KernelExecutable;
        kernelModuleInfo.Name  = kernelFile->string;
        kernelModuleInfo.Path  = kernelFile->path;

        kernelModuleInfo.LoadAddress = kernelFile->address;
        kernelModuleInfo.Size        = kernelFile->size;

        kernelModuleInfo.MediaType
            = kernelFile->media_type == LIMINE_MEDIA_TYPE_TFTP
                ? BootMediaType::eTFTP
            : kernelFile->media_type == LIMINE_MEDIA_TYPE_OPTICAL
                ? BootMediaType::eOptical
                : BootMediaType::eGeneric;
        kernelModuleInfo.TFTP_IPv4      = kernelFile->tftp_ip;
        kernelModuleInfo.TFTP_Port      = kernelFile->tftp_port;

        kernelModuleInfo.PartitionIndex = kernelFile->partition_index;
        kernelModuleInfo.MBR_DiskID     = kernelFile->mbr_disk_id;

        auto extractUUID                = [](const limine_uuid& uuid) -> UUID
        {
            u64 high = static_cast<u64>(uuid.a) << 32
                     | static_cast<u64>(uuid.b) << 16
                     | static_cast<u64>(uuid.c);

            u64 low = 0;
            Memory::Copy(&low, uuid.d, sizeof(u64));

            return {high, low};
        };

        kernelModuleInfo.GPT_DiskUUID = extractUUID(kernelFile->gpt_disk_uuid);
        kernelModuleInfo.GPT_PartitionUUID
            = extractUUID(kernelFile->gpt_part_uuid);
        kernelModuleInfo.FilesystemUUID = extractUUID(kernelFile->part_uuid);

        info.KernelModules              = {s_ModuleRequest.response->modules,
                                           s_ModuleRequest.response->module_count};
        info.KernelPhysicalBase
            = s_KernelAddressRequest.response->physical_base;
        info.KernelVirtualBase = s_KernelAddressRequest.response->virtual_base;

        info.MemoryMap         = s_MemoryMap;
        info.HigherHalfOffset  = s_HhdmRequest.response->offset;

        info.BspID             = s_SmpRequest.response->bsp_lapic_id;
        info.ProcessorCount    = s_SmpRequest.response->cpu_count;

        info.RSDP = Pointer(s_RsdpRequest.response->address).FromHigherHalf();
        info.DeviceTreeBlob
            = Pointer(s_DtbRequest.response->dtb_ptr).FromHigherHalf();
        info.SmBios32Phys
            = Pointer(s_SmbiosRequest.response->entry_32).FromHigherHalf();
        info.SmBios64Phys
            = Pointer(s_SmbiosRequest.response->entry_64).FromHigherHalf();

        info.EfiSystemTable       = s_EfiSystemTableRequest.response->address;
        info.EfiMemoryMap.Address = s_EfiMemmapRequest.response->memmap;
        info.EfiMemoryMap.Size    = s_EfiMemmapRequest.response->memmap_size;
        info.EfiMemoryMap.DescriptorSize
            = s_EfiMemmapRequest.response->desc_size;
        info.EfiMemoryMap.DescriptorVersion
            = s_EfiMemmapRequest.response->desc_version;

        kernelStart(info);
    }
    u64 GetHHDMOffset()
    {
        VerifyExistenceOrRetValue(s_HhdmRequest, 0);

        return s_HhdmRequest.response->offset;
    }
    Framebuffer** GetFramebuffers(usize& outCount)
    {
        VerifyExistenceOrRet(s_FramebufferRequest);

        outCount = s_FramebufferRequest.response->framebuffer_count;
        return s_FramebufferRequest.response->framebuffers;
    }
    Framebuffer* GetPrimaryFramebuffer()
    {
        VerifyExistenceOrRet(s_FramebufferRequest);
        return s_FramebufferRequest.response->framebuffers[0];
    }
    usize PagingMode()
    {
        VerifyExistenceOrRetValue(s_PagingModeRequest, 0);
        return s_PagingModeRequest.response->mode;
    }
    limine_mp_response* SMP_Response()
    {
        VerifyExistenceOrRet(s_SmpRequest);

        return s_SmpRequest.response;
    }
    limine_file* FindModule(const char* name)
    {
        VerifyExistenceOrRet(s_ModuleRequest);

        for (usize i = 0; i < s_ModuleRequest.response->module_count; i++)
        {
            if (!strcmp(s_ModuleRequest.response->modules[i]->string, name))
                return s_ModuleRequest.response->modules[i];
        }
        return nullptr;
    }
} // namespace BootInfo
