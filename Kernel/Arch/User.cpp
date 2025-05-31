/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/User.hpp>
#include <Debug/Assertions.hpp>

namespace Arch
{
    ErrorOr<void> CopyToUser(Pointer dest, Pointer src, usize count)
    {
        if (!InUserRange(dest, count)) return Error(EFAULT);

        Assert(!InUserRange(src, count));
        CPU::UserMemoryProtectionGuard guard;

        std::memcpy(dest.As<void>(), src.As<void>(), count);
        return {};
    }
    ErrorOr<void> CopyFromUser(Pointer dest, Pointer src, usize count)
    {
        if (!InUserRange(src, count)) return Error(EFAULT);
        Assert(!InUserRange(dest, count));
        CPU::UserMemoryProtectionGuard guard;

        std::memcpy(dest.As<void>(), src.As<void>(), count);
        return {};
    }
    ErrorOr<void> FillUser(Pointer buffer, isize value, usize count)
    {
        if (!InUserRange(buffer, count)) return Error(EFAULT);
        CPU::UserMemoryProtectionGuard guard;

        std::memset(buffer.As<void>(), value, count);
        return {};
    }
}; // namespace Arch
