/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>

export module Cryptix:Memory.MM;

export import :Boot.BootMemoryInfo;
export import :Memory.PageFault;
export import :Memory.PageTableEntry;
export import :Memory.Region;

export enum class MemoryUsage
{
    eGeneric    = 0,
    eKernelHeap = 1,
    eModule     = 2,
    eDMA        = 3,
    ePageTable  = 4,
    eDevice     = 5,
    eUser       = 6,
};

export struct MemoryMap;
export namespace MM
{
    void            PrepareInitialHeap(const BootMemoryInfo& memoryInfo);
    void            Initialize();

    Pointer         KernelPhysicalAddress();
    Pointer         KernelVirtualAddress();
    usize           GetHigherHalfOffset();
    enum PagingMode PagingMode();

    Ref<Region>     AllocateRegion(const usize    bytes,
                                   PageAttributes attributes = PageAttributes::eRW,
                                   const MemoryUsage memoryUsage
                                   = MemoryUsage::eGeneric);
    Ref<Region>     AllocateUserRegion(const usize bytes, PageAttributes flags);

    void            FreeRegion(Ref<Region> region);

    void            HandlePageFault(const PageFaultInfo& info);
}; // namespace MM
