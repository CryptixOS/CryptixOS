/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

class ZeroDevice final : public CharacterDevice
{
  public:
    ZeroDevice()
        : CharacterDevice("zero", MakeDevice(1, 5))
    {
    }
    virtual StringView     Name() const noexcept override { return "zero"; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        Memory::Fill(dest, 0, bytes);

        return bytes;
    }
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        Memory::Fill(out.Raw(), 0, count);
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
