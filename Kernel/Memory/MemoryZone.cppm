/*
 * Created by v1tr10l7 on 13.07.2025.
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

export module Cryptix:Memory.MemoryZone;

export import :Memory.Region;

export enum class MemoryZoneType
{
    eUsable                = 0,
    eReserved              = 1,
    eACPI_Reclaimable      = 2,
    eACPI_NVS              = 3,
    eBadMemory             = 4,
    eBootloaderReclaimable = 5,
    eKernelAndModules      = 6,
    eFramebuffer           = 7,
    eCount                 = 8,
};
export class MemoryZone
{
  public:
    constexpr MemoryZone() = default;
    constexpr MemoryZone(Pointer base, usize length,
                         MemoryZoneType type = MemoryZoneType::eUsable)
        : m_Base(base)
        , m_Length(length)
        , m_Type(type)
    {
    }

    constexpr Pointer Allocate(usize bytes)
    {
        if (bytes > Length() || Type() != MemoryZoneType::eUsable)
            return nullptr;
        auto memory = Base();

        m_Base += bytes;
        m_Length -= bytes;
        return memory;
    }

    constexpr Pointer        Base() const { return m_Base; }
    constexpr usize          Length() const { return m_Length; }
    constexpr MemoryZoneType Type() const { return m_Type; }

  private:
    Pointer        m_Base   = nullptr;
    usize          m_Length = 0;
    MemoryZoneType m_Type   = MemoryZoneType::eReserved;
};
