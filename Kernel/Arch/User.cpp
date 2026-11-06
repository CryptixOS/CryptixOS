/*
 * Created by v1tr10l7 on 30.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/User.hpp>
#include <Debug/Assertions.hpp>

namespace
{
    // Whether supervisor access to user pages is currently allowed.
    bool IsUserAccessEnabled()
    {
#ifdef CTOS_TARGET_X86_64
        // RFLAGS.AC (bit 18) is what stac/clac toggle for SMAP. It is part of
        // RFLAGS, so it is saved and restored per thread across preemption.
        u64 flags;
        __asm__ volatile("pushfq\n\tpopq %0" : "=r"(flags));
        return flags & (1ull << 18);
#else
        // TODO(v1tr10l7): query the TTBR0-switching state on aarch64. Until
        // then keep the old behaviour (always disable on guard exit).
        return false;
#endif
    }
} // namespace

UserMemoryProtectionGuard::UserMemoryProtectionGuard()
    : m_WasEnabled(IsUserAccessEnabled())
{
    if (!m_WasEnabled) CPU::EnableUserAccess();
}
UserMemoryProtectionGuard::~UserMemoryProtectionGuard()
{
    if (!m_WasEnabled) CPU::DisableUserAccess();
}

ErrorOr<void> CopyToUser(Pointer dest, Pointer src, usize count)
{
    if (!InUserRange(dest, count)) return Error(EFAULT);

    Assert(!InUserRange(src, count));
    UserMemoryProtectionGuard guard;

    Memory::Copy(dest.As<void>(), src.As<void>(), count);
    return {};
}
ErrorOr<void> CopyFromUser(Pointer dest, Pointer src, usize count)
{
    if (!InUserRange(src, count)) return Error(EFAULT);
    Assert(!InUserRange(dest, count));
    UserMemoryProtectionGuard guard;

    Memory::Copy(dest.As<void>(), src.As<void>(), count);
    return {};
}
ErrorOr<void> FillUser(Pointer buffer, isize value, usize count)
{
    if (!InUserRange(buffer, count)) return Error(EFAULT);
    UserMemoryProtectionGuard guard;

    Memory::Fill(buffer.As<void>(), value, count);
    return {};
}
