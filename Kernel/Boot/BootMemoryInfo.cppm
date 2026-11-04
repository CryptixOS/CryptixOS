/*
 * Created by v1tr10l7 on 15.07.2025.
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

export module Cryptix:Boot.BootMemoryInfo;

export import :Memory.MemoryZone;

export enum class PagingMode
{
    eNone   = 0,
    eLevel4 = 4,
    eLevel5 = 5,
};
export struct MemoryMap
{
    MemoryZone* Entries    = nullptr;
    usize       EntryCount = 0;
};
export struct EfiMemoryMap
{
    Pointer Address           = nullptr;
    usize   Size              = 0;
    usize   DescriptorSize    = 0;
    u32     DescriptorVersion = 0;
};

export struct BootMemoryInfo
{
    Pointer             KernelPhysicalBase = nullptr;
    Pointer             KernelVirtualBase  = nullptr;
    usize               HigherHalfOffset   = 0;
    enum PagingMode     PagingMode         = PagingMode::eNone;
    struct MemoryMap    MemoryMap          = {nullptr, 0};
    struct EfiMemoryMap EfiMemoryMap;
};
