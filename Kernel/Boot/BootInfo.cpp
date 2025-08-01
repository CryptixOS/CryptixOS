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
#include <Prism/Memory/Memory.hpp>

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

    static BootInformation* s_BootInformation;

#define HigherHalfPointer(addr) addr.Offset(s_HhdmRequest.response->offset)
    Pointer AllocateBootMemory(usize bytes)
    {
        auto response = s_MemmapRequest.response;
        Assert(response);
        usize entryCount = response->entry_count;
        auto  entries    = response->entries;

        for (usize i = 0; i < entryCount; i++)
        {
            bytes       = Math::AlignUp(bytes, PMM::PAGE_SIZE);
            auto* entry = entries[i];
            if (entry->type != LIMINE_MEMMAP_USABLE || entry->length < bytes)
                continue;

            Pointer address = entry->base;
            entry->base += bytes;
            entry->length -= bytes;

            s_BootInformation->LowestAllocatedAddress
                = Min(s_BootInformation->LowestAllocatedAddress, address);
            s_BootInformation->HighestAllocatedAddress
                = Max(s_BootInformation->HighestAllocatedAddress, address);
            return HigherHalfPointer(address);
        }

        return nullptr;
    };
    enum FirmwareType FirmwareType()
    {
        switch (s_FirmwareTypeRequest.response->firmware_type)
        {
            case LIMINE_FIRMWARE_TYPE_X86BIOS:
                return FirmwareType::eX86Bios;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI32:
                return FirmwareType::eUefi32;
                break;
            case LIMINE_FIRMWARE_TYPE_UEFI64:
                return FirmwareType::eUefi64;
                break;
            case LIMINE_FIRMWARE_TYPE_SBI: return FirmwareType::eSbi; break;

            default: break;
        }

        return FirmwareType::eUndefined;
    }
    enum PagingMode PagingMode()
    {
        auto response = s_PagingModeRequest.response;
        if (!response) return PagingMode::eNone;

        switch (response->mode)
        {
#ifdef CTOS_TARGET_X86_64
            case LIMINE_PAGING_MODE_X86_64_4LVL:
#elifdef CTOS_TARGET_AARCH64
            case LIMINE_PAGING_MODE_AARCH64_4LVL:
#endif
                return PagingMode::eLevel4;
#ifdef CTOS_TARGET_X86_64
            case LIMINE_PAGING_MODE_X86_64_5LVL:
#elifdef CTOS_TARGET_AARCH64
            case LIMINE_PAGING_MODE_AARCH64_5LVL:
#endif
                return PagingMode::eLevel5;

            default: break;
        }

        return PagingMode::eNone;
    }

    void InitializeModule(BootModuleInfo& module, limine_file* file)
    {
        static auto extractUUID = [](const limine_uuid& uuid) -> UUID
        {
            u64 high = static_cast<u64>(uuid.a) << 32
                     | static_cast<u64>(uuid.b) << 16
                     | static_cast<u64>(uuid.c);

            u64 low = 0;
            Memory::Copy(&low, uuid.d, sizeof(u64));

            return {high, low};
        };

        module.Name              = file->string;
        module.Path              = file->path;
        module.LoadAddress       = file->address;
        module.Size              = file->size;
        module.MediaType         = file->media_type == LIMINE_MEDIA_TYPE_TFTP
                                     ? BootMediaType::eTFTP
                                 : file->media_type == LIMINE_MEDIA_TYPE_OPTICAL
                                     ? BootMediaType::eOptical
                                     : BootMediaType::eGeneric;
        module.TFTP_IPv4         = file->tftp_ip;
        module.TFTP_Port         = file->tftp_port;
        module.PartitionIndex    = file->partition_index;
        module.MBR_DiskID        = file->mbr_disk_id;
        module.GPT_DiskUUID      = extractUUID(file->gpt_disk_uuid);
        module.GPT_PartitionUUID = extractUUID(file->gpt_part_uuid);
        module.FilesystemUUID    = extractUUID(file->part_uuid);
    }
    void InitializeBootModules(Span<BootModuleInfo, DynamicExtent>& modulesInfo)
    {
        auto bootModuleResponse = s_ModuleRequest.response;
        if (!bootModuleResponse) return;

        usize moduleCount = bootModuleResponse->module_count;
        auto  modules     = bootModuleResponse->modules;

        usize bytes       = moduleCount * sizeof(BootModuleInfo);
        auto  bootModules = AllocateBootMemory(bytes).As<BootModuleInfo>();

        modulesInfo       = Span<BootModuleInfo>(bootModules, moduleCount);
        for (usize i = 0; i < moduleCount; i++)
        {
            auto& module = modulesInfo[i];
            auto  file   = modules[i];
            InitializeModule(module, file);
        }
    }
    void SetupMemoryInfo(BootMemoryInfo& info)
    {
        auto response = s_MemmapRequest.response;
        Assert(response);
        auto kernelAddressResponse = s_KernelAddressRequest.response;
        Assert(kernelAddressResponse);

        auto entries                  = response->entries;
        auto entryCount               = response->entry_count;

        auto fromLimineMemoryZoneType = [](u8 type) -> MemoryZoneType
        {
            switch (type)
            {
                case LIMINE_MEMMAP_USABLE: return MemoryZoneType::eUsable;
                case LIMINE_MEMMAP_RESERVED: return MemoryZoneType::eReserved;
                case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                    return MemoryZoneType::eACPI_Reclaimable;
                case LIMINE_MEMMAP_ACPI_NVS: return MemoryZoneType::eACPI_NVS;
                case LIMINE_MEMMAP_BAD_MEMORY:
                    return MemoryZoneType::eBadMemory;
                case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                    return MemoryZoneType::eBootloaderReclaimable;
                case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
                    return MemoryZoneType::eKernelAndModules;
                case LIMINE_MEMMAP_FRAMEBUFFER:
                    return MemoryZoneType::eFramebuffer;

                default: break;
            }

            return MemoryZoneType::eReserved;
        };

        auto hhdmResponse = s_HhdmRequest.response;
        Assert(hhdmResponse);

        info.KernelPhysicalBase = kernelAddressResponse->physical_base;
        info.KernelVirtualBase  = kernelAddressResponse->virtual_base;
        info.HigherHalfOffset   = hhdmResponse->offset;
        info.PagingMode         = PagingMode();

        info.MemoryMap.Entries
            = AllocateBootMemory(sizeof(MemoryZone) * entryCount);
        info.MemoryMap.EntryCount = entryCount;

        for (usize i = 0; i < entryCount; i++)
        {
            auto*          current    = entries[i];
            u64            base       = current->base;
            u64            length     = current->length;
            MemoryZoneType type       = fromLimineMemoryZoneType(current->type);

            info.MemoryMap.Entries[i] = MemoryZone(base, length, type);
        }

        auto efiMemmapResponse = s_EfiMemmapRequest.response;
        if (efiMemmapResponse)
        {
            info.EfiMemoryMap.Address        = efiMemmapResponse->memmap;
            info.EfiMemoryMap.Size           = efiMemmapResponse->memmap_size;
            info.EfiMemoryMap.DescriptorSize = efiMemmapResponse->desc_size;
            info.EfiMemoryMap.DescriptorVersion
                = efiMemmapResponse->desc_version;
        }
    }
    void SetupFramebuffers(Span<Framebuffer, DynamicExtent>& out)
    {
        auto framebufferResponse = s_FramebufferRequest.response;
        if (!framebufferResponse) return;

        usize framebufferCount = framebufferResponse->framebuffer_count;
        auto  framebuffers     = framebufferResponse->framebuffers;

        usize framebuffersSize = framebufferCount * sizeof(Framebuffer);
        out                    = Span<Framebuffer>(
            AllocateBootMemory(framebuffersSize).As<Framebuffer>(),
            framebufferCount);

        auto setupVideoModes = [](Framebuffer& out, limine_framebuffer* current)
        {
            usize videoModeCount = current->mode_count;
            if (videoModeCount == 0) return;

            auto  videoModes     = current->modes;
            usize videoModesSize = videoModeCount * sizeof(VideoMode);
            out.VideoModes
                = Span(AllocateBootMemory(videoModesSize).As<VideoMode>(),
                       videoModeCount);
            for (usize i = 0; i < videoModeCount; i++)
            {
                auto  videoMode      = videoModes[i];
                auto& modeOut        = out.VideoModes[i];
                modeOut.Pitch        = videoMode->pitch;
                modeOut.Width        = videoMode->width;
                modeOut.Height       = videoMode->height;
                modeOut.BitsPerPixel = videoMode->bpp;
                modeOut.MemoryModel
                    = videoMode->memory_model == LIMINE_FRAMEBUFFER_RGB
                        ? MemoryModel::eRGB
                        : MemoryModel::eRGB;
                modeOut.RedMaskSize    = videoMode->red_mask_size;
                modeOut.RedMaskShift   = videoMode->red_mask_shift;
                modeOut.GreenMaskSize  = videoMode->green_mask_size;
                modeOut.GreenMaskShift = videoMode->green_mask_shift;
                modeOut.BlueMaskSize   = videoMode->blue_mask_size;
                modeOut.BlueMaskShift  = videoMode->blue_mask_shift;
            }
        };

        for (usize i = 0; i < framebufferCount; i++)
        {
            auto  current           = framebuffers[i];
            auto& currentOut        = out[i];

            currentOut.Address      = current->address;
            currentOut.Width        = current->width;
            currentOut.Height       = current->height;
            currentOut.Pitch        = current->pitch;
            currentOut.BitsPerPixel = current->bpp;
            currentOut.MemoryModel
                = current->memory_model == LIMINE_FRAMEBUFFER_RGB
                    ? MemoryModel::eRGB
                    : MemoryModel::eRGB;
            currentOut.RedMaskSize    = current->red_mask_size;
            currentOut.RedMaskShift   = current->red_mask_shift;
            currentOut.GreenMaskSize  = current->green_mask_size;
            currentOut.GreenMaskShift = current->green_mask_shift;
            currentOut.BlueMaskSize   = current->blue_mask_size;
            currentOut.BlueMaskShift  = current->blue_mask_shift;
            currentOut.EDID_Size      = current->edid_size;
            currentOut.EDID           = current->edid;

            setupVideoModes(currentOut, current);
        }
    }

    void SetupBootInfo(BootInformation& info)
    {
        s_BootInformation              = &info;
        auto bootInfoResponse          = s_BootloaderInfoRequest.response;
        auto executableCmdlineResponse = s_ExecutableCmdlineRequest.response;
        auto dateAtBootResponse        = s_DateAtBootRequest.response;

        if (bootInfoResponse)
        {
            info.BootloaderName    = bootInfoResponse->name;
            info.BootloaderVersion = bootInfoResponse->version;
        }
        if (executableCmdlineResponse)
            info.KernelCommandLine = executableCmdlineResponse->cmdline;

        info.FirmwareType = FirmwareType();
        if (dateAtBootResponse) info.DateAtBoot = dateAtBootResponse->timestamp;

        auto executableResponse = s_ExecutableRequest.response;
        if (executableResponse)
            InitializeModule(info.KernelExecutable,
                             executableResponse->executable_file);
        InitializeBootModules(info.KernelModules);
        SetupFramebuffers(info.Framebuffers);
        // Parsing memory entries needs to happen last, so every allocation
        // happens before that
        SetupMemoryInfo(info.MemoryInformation);

        auto smpResponse = s_SmpRequest.response;
        if (smpResponse)
        {
#ifdef CTOS_TARGET_X86_64
            info.BspID = smpResponse->bsp_lapic_id;
#elifdef CTOS_TARGET_AARCH64
            info.BspID = smpResponse->bsp_mpidr;
#endif
            info.ProcessorCount = smpResponse->cpu_count;
        }

#define ToPhysAddr(higher_half_address)                                        \
    Pointer(higher_half_address).Offset(-s_HhdmRequest.response->offset)
        auto rsdpResponse = s_RsdpRequest.response;
        if (rsdpResponse) info.RSDP = ToPhysAddr(rsdpResponse->address);

        auto dtbResponse = s_DtbRequest.response;
        if (dtbResponse) info.DeviceTreeBlob = ToPhysAddr(dtbResponse->dtb_ptr);

        auto smbiosResponse = s_SmbiosRequest.response;
        if (smbiosResponse)
        {
            info.SmBios32Phys = ToPhysAddr(smbiosResponse->entry_32);
            info.SmBios64Phys = ToPhysAddr(smbiosResponse->entry_64);
        }

        auto efiSystemTableResponse = s_EfiSystemTableRequest.response;
        if (efiSystemTableResponse)
            info.EfiSystemTable = ToPhysAddr(efiSystemTableResponse->address);
    }
} // namespace

extern "C" CTOS_NO_KASAN [[noreturn]] void kernelStart(const BootInformation&);

#define VerifyExistenceOrRet(requestName)                                      \
    if (!requestName.response) return nullptr;
#define VerifyExistenceOrRetValue(requestName, value)                          \
    if (!requestName.response) return value;

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

    if (!s_MemmapRequest.response || s_MemmapRequest.response->entry_count == 0)
        Panic("Boot: Failed to acquire limine memory map entries");
    if (!s_FramebufferRequest.response
        || s_FramebufferRequest.response->framebuffer_count < 1)
        EarlyPanic("Boot: Failed to acquire the framebuffer!");
    Logger::EnableSink(LOG_SINK_TERMINAL);

    BootInformation info;
    Memory::Fill(&info, 0, sizeof(info));

    SetupBootInfo(info);

    kernelStart(info);
}
limine_mp_response* SMP_Response()
{
    VerifyExistenceOrRet(s_SmpRequest);

    return s_SmpRequest.response;
}
