/*
 * Created by v1tr10l7 on 13.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/Region.hpp>

enum class MemoryZoneType
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
class MemoryZone
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

    constexpr Pointer        Base() const { return m_Base; }
    constexpr usize          Length() const { return m_Length; }
    constexpr MemoryZoneType Type() const { return m_Type; }

  private:
    Pointer        m_Base   = nullptr;
    usize          m_Length = 0;
    MemoryZoneType m_Type   = MemoryZoneType::eReserved;
};
