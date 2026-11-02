/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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

export module Cryptix:Arch.CPU;

#if defined(CTOS_TARGET_X86_64)
export import :Arch.x86_64.CPU;
#endif
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
export import :Arch.aarch64.CPU;
#endif
export import :Memory.VMM;

#ifdef CTOS_TARGET_X86_64
#elifdef CTOS_TARGET_AARCH64
#endif

export class ClockSource;
export struct Thread;
export struct ExecutionContext;
export namespace CPU
{
    constexpr usize KERNEL_STACK_SIZE = 64_kib;
    constexpr usize USER_STACK_SIZE   = 2_mib;

    bool            GetInterruptFlag();
    void            SetInterruptFlag(bool enabled);
    bool            SwapInterruptFlag(bool enabled);

    u64             GetOnlineCPUsCount();
    struct CPU;
    CPU*         Current();
    u64          GetCurrentID();
    CPU*         GetCurrent();

    ClockSource* HighResolutionClock();

    Thread*      GetCurrentThread();

    void         PrepareThread(Thread* thread, Pointer pc, Pointer arg = 0);

    void         SaveThread(Thread* thread, ExecutionContext* ctx);
    void         LoadThread(Thread* thread, ExecutionContext* ctx);

    void         Reschedule(Timestep us);

    void         HaltAll();
    void         WakeUp(usize id, bool everyone);

    bool         DuringSyscall();
    void         OnSyscallEnter(usize index);
    void         OnSyscallLeave();
}; // namespace CPU
