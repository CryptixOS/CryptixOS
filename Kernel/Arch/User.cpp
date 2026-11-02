/*
 * Created by v1tr10l7 on 30.05.2025.
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
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>

module Cryptix:Impl.Arch.User;

import :Arch.CPU;
import :Arch.User;
import :Debug.Assertions;

UserMemoryProtectionGuard::UserMemoryProtectionGuard()
{
    CPU::EnableUserAccess();
}
UserMemoryProtectionGuard::~UserMemoryProtectionGuard()
{
    CPU::DisableUserAccess();
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
