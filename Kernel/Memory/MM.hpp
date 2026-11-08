/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootMemoryInfo.hpp>

#include <Memory/PageFault.hpp>
#include <Memory/PageTableEntry.hpp>
#include <Memory/Region.hpp>

#include <Prism/Core/Types.hpp>

enum class MemoryUsage
{
    eGeneric    = 0,
    eKernelHeap = 1,
    eModule     = 2,
    eDMA        = 3,
    ePageTable  = 4,
    eDevice     = 5,
    eUser       = 6,
};

struct MemoryMap;
namespace MM
{
    void            PrepareInitialHeap(const BootMemoryInfo& memoryInfo);
    void            Initialize();

    // Physical memory in bootloader-reclaimable ranges that is still in use
    // and must survive FinishInitialization(): the device tree blob, and
    // each CPU's boot stack (NoteBootStack(), from that CPU's entry point)
    void            KeepPhysicalRange(Pointer base, usize size);
    void            NoteBootStack(Pointer stackPointer);

    // Once the kernel is up, before init runs (Linux's free_initmem() and
    // mark_rodata_ro()): gives the bootloader's memory and .kernel_init to
    // the PMM, makes .ro_after_init read-only and prints a memory summary
    void            FinishInitialization();

    Pointer         KernelPhysicalAddress();
    Pointer         KernelVirtualAddress();
    usize           HigherHalfOffset();
    enum PagingMode PagingMode();

    Ref<Region>     AllocateRegion(const usize    bytes,
                                   PageAttributes attributes = PageAttributes::eRW,
                                   const MemoryUsage memoryUsage
                                   = MemoryUsage::eGeneric);
    Ref<Region>     AllocateUserRegion(const usize bytes, PageAttributes flags);

    void            FreeRegion(Ref<Region> region);

    void            HandlePageFault(const PageFaultInfo& info);
}; // namespace MM

