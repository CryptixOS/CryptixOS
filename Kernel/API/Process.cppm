/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/signal.h>
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
#include <API/UnixTypes.hpp>
#include <API/Posix/linux/time.h>
#include <API/Posix/sys/resource.h>
#include <API/Posix/linux/sched.h>

export module Cryptix:API.Process;

export import :API.Syscall;

export namespace API::Process
{
    ErrorOr<isize>     SigAction(isize signal, const sigaction* action,
                                 sigaction* oldAction);
    ErrorOr<isize>     SigProcMask(i32 how, const sigset_t* newSet,
                                   sigset_t* oldSet, usize sigSetSize);
    ErrorOr<isize>     SigReturn();
    ErrorOr<isize>     SchedYield();

    ErrorOr<pid_t>     Pid();
    ErrorOr<ProcessID> Clone(usize flags, usize newSp, i32* parentTid,
                             i32* childTid, usize tls);
    ErrorOr<ProcessID> Fork();
    ErrorOr<ProcessID> VFork();
    ErrorOr<isize>     Execve(char* pathname, char** argv, char** envp);
    ErrorOr<isize>     Exit(isize exitcode);
    ErrorOr<isize>     Wait4(pid_t pid, isize* wstatus, isize flags,
                             rusage* rusage);
    ErrorOr<isize>     Kill(pid_t pid, isize signal);

    ErrorOr<mode_t>    Umask(mode_t mask);

    ErrorOr<uid_t>     GetUid();
    ErrorOr<gid_t>     GetGid();

    ErrorOr<isize>     SetUid(uid_t uid);
    ErrorOr<isize>     SetGid(gid_t gid);

    ErrorOr<uid_t>     GetEUid();
    ErrorOr<gid_t>     GetEGid();

    ErrorOr<isize>     SetPGid(pid_t pid, pid_t pgid);
    ErrorOr<pid_t>     GetPPid();
    ErrorOr<pid_t>     GetPGrp(pid_t pid);

    ErrorOr<pid_t>     SetSid();
    ErrorOr<isize>     SetReUid(uid_t ruid, uid_t euid);
    ErrorOr<isize>     SetReGid(gid_t rgid, gid_t egid);
    ErrorOr<isize>     SetResUid(uid_t ruid, uid_t euid, uid_t suid);
    ErrorOr<isize>     GetResUid(uid_t* ruid, uid_t* euid, uid_t* suid);
    ErrorOr<isize>     SetResGid(gid_t rgid, gid_t egid, gid_t sgid);
    ErrorOr<isize>     GetResGid(gid_t* rgid, gid_t* egid, gid_t* sgid);

    ErrorOr<pid_t>     GetPGid(pid_t pid);
    ErrorOr<pid_t>     GetSid(pid_t pid);

    ErrorOr<isize>     SigSuspend(const sigset_t* mask);
    ErrorOr<isize>     SigAltStack(const sigaltstack* ss,
                                   sigaltstack*              oldSs);
    ErrorOr<ThreadID>  GetTid();

    ErrorOr<isize>     Futex(u32* uaddr, isize op, u32 expected,
                             const struct timespec* utime, u32* uaddr2, u32 value2);
    ErrorOr<isize>     Clone3(clone_args* uargs, usize size);
    ErrorOr<isize>     FutexWaitV(struct futex_waitv* waiters, usize futexCount,
                                  usize flags, struct timespec* timeout,
                                  clockid_t clockid);
    ErrorOr<usize> FutexWake(i32* uaddr, usize mask, isize count, usize flags);
    ErrorOr<usize> FutexWait(i32* uaddr, usize value, usize mask, usize flags,
                             struct timespec* timeout, clockid_t clockid);
    ErrorOr<isize> FutexRequeue(struct futex_waitv* waiters, usize flags,
                                isize wakeCount, isize requeueCount);
} // namespace API::Process
