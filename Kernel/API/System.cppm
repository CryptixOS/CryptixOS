/*
 * Created by v1tr10l7 on 15.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/resource.h>
#include <API/Posix/sys/utsname.h>
#include <API/Syscall.hpp>
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
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>

export module Cryptix:API.System;

export import :API.Syscall;

export namespace API::System
{
    enum class RebootCommand : isize
    {
        eRestart   = 0,
        eHalt      = 1,
        ePowerOff  = 2,
        eRestart2  = 3,
        eSuspend   = 4,
        eKexec     = 5,
        eUndefined = -1,
    };

    ErrorOr<isize>    Uname(utsname* out);
    ErrorOr<isize>    GetResourceLimit(isize resource, rlimit* rlimit);
    ErrorOr<isize>    GetResourceUsage(isize who, rusage* usage);

    ErrorOr<isize>    PrCtl(isize opcode, upointer arg1, upointer arg2,
                            upointer arg3, upointer arg4);
    ErrorOr<isize>    ArchPrCtl(isize opcode, upointer arg1);

    ErrorOr<isize>    Reboot(RebootCommand cmd);
    ErrorOr<isize>    InitModule(upointer image, usize size,
                                 const char** parameters);

    ErrorOr<upointer> SysPanic(const char* errorMessage);
    ErrorOr<isize>    DebugLog(const char* message);
} // namespace API::System
