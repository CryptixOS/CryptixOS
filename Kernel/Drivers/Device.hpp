/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "API/UnixTypes.hpp"

constexpr dev_t makeDevice(usize major, usize minor)
{
    dev_t dev = dev_t(major & 0x00000fffU) << 8;
    dev |= dev_t(major & 0xfffff000U) << 32;
    dev |= dev_t(minor & 0x000000ffU);
    dev |= dev_t(minor & 0xffffff00U) << 12;

    return dev;
}

enum class DriverType
{
    eUndefined    = 0,
    eMemoryDevice = 1,
    ePTY          = 2,
    eTTY          = 4,
};
enum class DeviceType
{
    eNull = 3,
    eZero = 5,
    eFull = 7,
};

constexpr dev_t makeDevice(DriverType major, DeviceType minor)
{
    return makeDevice(static_cast<usize>(major), static_cast<usize>(minor));
}

class Device
{
  public:
    Device(DriverType major, DeviceType minor)
        : id(makeDevice(major, minor))
    {
    }

    inline dev_t             GetID() const noexcept { return id; }
    virtual std::string_view GetName() const noexcept                    = 0;

    virtual isize            Read(void* dest, off_t offset, usize bytes) = 0;
    virtual isize Write(const void* src, off_t offset, usize bytes)      = 0;

    virtual i32   IoCtl(usize request, uintptr_t argp)                   = 0;

  protected:
    dev_t id;
};
