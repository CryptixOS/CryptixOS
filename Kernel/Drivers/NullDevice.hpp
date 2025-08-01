/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

class NullDevice final : public CharacterDevice
{
  public:
    NullDevice()
        : CharacterDevice("null", MakeDevice(1, 0))
    {
    }
    virtual StringView     Name() const noexcept override { return "null"; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        return EOF;
    }
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return EOF;
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        return bytes;
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return count;
    }

    virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }
};
