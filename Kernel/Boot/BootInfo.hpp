/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/PMM.hpp>

#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/Memory/Pointer.hpp>
#include <Prism/Network/Ipv4Address.hpp>

#include <Prism/Core/UUID.hpp>
#include <Prism/Utility/PathView.hpp>
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

enum class BootMediaType
{
    eGeneric = 0,
    eOptical = 1,
    eTFTP    = 2,
};
struct BootModuleInfo
{
    StringView    Name              = ""_sv;
    PathView      Path              = ""_sv;

    // Virtual address to where the module was loaded
    Pointer       LoadAddress       = nullptr;
    usize         Size              = 0;

    BootMediaType MediaType         = BootMediaType::eGeneric;
    IPv4Address   TFTP_IPv4         = "127.0.0.1"_sv;
    u32           TFTP_Port         = 21;

    usize         PartitionIndex    = 0;
    usize         MBR_DiskID        = 0;
    UUID          GPT_DiskUUID      = 0;
    UUID          GPT_PartitionUUID = 0;
    UUID          FilesystemUUID    = 0;
};

struct BootInformation
{
    StringView                        BootloaderName    = ""_sv;
    StringView                        BootloaderVersion = ""_sv;
    StringView                        KernelCommandLine = ""_sv;
    enum FirmwareType                 FirmwareType;
    DateTime                          DateAtBoot = 0;

    BootModuleInfo                    KernelExecutable{};
    Span<limine_file*, DynamicExtent> KernelModules;
    Pointer                           KernelPhysicalBase = nullptr;
    Pointer                           KernelVirtualBase  = nullptr;

    MemoryMap                         MemoryMap          = {nullptr, 0};
    u8                                PagingMode         = 0;
    usize                             HigherHalfOffset   = 0;

    u64                               BspID              = 0;
    u64                               ProcessorCount     = 0;
    Span<Framebuffer, DynamicExtent>  Framebuffers;

    // Physical address of the RSDP structure
    Pointer                           RSDP           = nullptr;
    // Physical address of the Device Tree Blob
    Pointer                           DeviceTreeBlob = nullptr;
    // Physical address of the SMBIOS 32 bit EntryPoint structure
    Pointer                           SmBios32Phys   = nullptr;
    // Physical address of the SMBIOS 64 bit EntryPoint structure
    Pointer                           SmBios64Phys   = nullptr;

    Pointer                           EfiSystemTable = nullptr;
    struct EfiMemoryMap               EfiMemoryMap;
};

namespace BootInfo
{
    u64                 GetHHDMOffset();
    Framebuffer**       GetFramebuffers(usize& outCount);
    Framebuffer*        GetPrimaryFramebuffer();
    usize               PagingMode();
    limine_mp_response* SMP_Response();

    limine_file*        FindModule(const char* name);
}; // namespace BootInfo
