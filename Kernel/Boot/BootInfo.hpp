/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Time.hpp>

#include <limine.h>

constexpr u32 FRAMEBUFFER_MEMORY_MODEL_RGB = LIMINE_FRAMEBUFFER_RGB;

using Framebuffer                          = limine_framebuffer;
enum class MemoryType
{
    eUsable                = 0,
    eReserved              = 1,
    eACPI_Reclaimable      = 2,
    eACPI_NVS              = 3,
    eBadMemory             = 4,
    eBootloaderReclaimable = 5,
    eKernelAndModules      = 6,
    eFramebuffer           = 7,
};
struct MemoryRegion
{
  public:
    constexpr MemoryRegion(Pointer base, usize size, MemoryType type)
        : m_Base(base)
        , m_Size(size)
        , m_Type(type)
    {
    }

    constexpr Pointer    Base() const { return m_Base; }
    constexpr usize      Size() const { return m_Size; }
    constexpr MemoryType Type() const { return m_Type; }

    Pointer              m_Base = 0;
    usize                m_Size = 0;
    MemoryType           m_Type = MemoryType::eReserved;
};
struct MemoryMap
{
    MemoryRegion* Entries    = nullptr;
    usize         EntryCount = 0;
};

enum class FirmwareType : i32
{
    eUndefined = -1,
    eX86Bios   = 0,
    eUefi32    = 1,
    eUefi64    = 2,
    eSbi       = 3,
};

struct BootInformation
{
    StringView        BootloaderName    = ""_sv;
    StringView        BootloaderVersion = ""_sv;
    StringView        KernelCommandLine = ""_sv;
    enum FirmwareType FirmwareType;
    DateTime          DateAtBoot         = 0;

    limine_file*      KernelExecutable   = nullptr;
    limine_file**     KernelModules      = nullptr;
    Pointer           KernelPhysicalBase = nullptr;
    Pointer           KernelVirtualBase  = nullptr;

    MemoryMap         MemoryMap          = {nullptr, 0};
    u8                PagingMode         = 0;
    usize             HigherHalfOffset   = 0;

    u64               BspID              = 0;
    u64               ProcessorCount     = 0;
    Span<Framebuffer> Framebuffers;

    Pointer           RSDP                       = nullptr;
    Pointer           DeviceTree                 = nullptr;
    Pointer           SmBios32                   = nullptr;
    Pointer           SmBios64                   = nullptr;

    Pointer           EfiSystemTable             = nullptr;
    Pointer           EfiMemoryMap               = nullptr;
    usize             EfiMemoryMapSize           = 0;
    usize             EfiMemoryDescriptorSize    = 0;
    usize             EfiMemoryDescriptorVersion = 0;
};

namespace BootInfo
{
    StringView                  BootloaderName();
    StringView                  BootloaderVersion();
    StringView                  KernelCommandLine();
    enum FirmwareType           FirmwareType();

    u64                         GetHHDMOffset();
    Framebuffer**               GetFramebuffers(usize& outCount);
    Framebuffer*                GetPrimaryFramebuffer();
    usize                       PagingMode();
    limine_mp_response*         SMP_Response();
    MemoryMap&                  MemoryMap();

    limine_file*                ExecutableFile();
    limine_file*                FindModule(const char* name);

    Pointer                     RSDPAddress();
    std::pair<Pointer, Pointer> GetSmBiosEntries();
    Pointer                     EfiSystemTable();
    limine_efi_memmap_response* EfiMemoryMap();

    u64                         DateAtBoot();
    Pointer                     KernelPhysicalAddress();
    Pointer                     KernelVirtualAddress();

    Pointer                     DeviceTreeBlobAddress();
}; // namespace BootInfo
