/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Singleton.hpp>
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
#include <API/Posix/linux/capability.h>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

export module Cryptix:Scheduler.Scheduler;

export import :Scheduler.Process;

export class Process;
export struct Thread;
export class Scheduler
{
  public:
    KERNEL_INIT_CODE
    static void     Initialize();
    static void     PrepareAP(bool start = false);

    static bool     IsPreemptionEnabled();

    static void     EnablePreemption();
    static void     DisablePreemption();

    static void     Block(Thread* thread);
    static void     Unblock(Thread* thread);

    static void     Yield(bool saveCtx = false);

    static Process* KernelProcess();
    static Process* GetKernelProcess();

    static Process* CreateProcess(Process* parent, StringView name,
                                  const Credentials& creds);
    static void     RemoveProcess(ProcessID pid);

    static bool     ValidatePid(ProcessID pid);
    static Process* GetProcess(ProcessID pid);

    static void     EnqueueThread(Thread* thread);
    static void     EnqueueNotReady(Thread* thread);
    static void     DequeueThread(Thread* thread);

  private:
    Scheduler() = default;

    static Process::List& ProcessList();
    friend class Process;

    static Thread* GetNextThread(usize cpuID);
    static Thread* PickReadyThread();
    static void    SwitchContext(Thread*                  newThread,
                                 struct ExecutionContext* context);

    static void    Tick(struct ExecutionContext*);
}; // namespace Scheduler
