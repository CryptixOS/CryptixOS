/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/signal.h>
#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

struct timespec;
struct rusage;
struct clone_args;
namespace API::Process
{
    ErrorOr<isize>     SigAction(isize signal, const struct sigaction* action,
                                 sigaction* oldAction);
    ErrorOr<isize>     SigProcMask(i32 how, const sigset_t* newSet,
                                   sigset_t* oldSet);
    ErrorOr<isize>     SigReturn();
    ErrorOr<isize>     SchedYield();
    ErrorOr<isize>     NanoSleep(const timespec* duration, timespec* rem);

    ErrorOr<pid_t>     Pid();
    ErrorOr<ProcessID> Clone(usize flags, usize newSp, i32* parentTid,
                             i32* childTid, usize tls);
    ErrorOr<ProcessID> Fork();
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
    ErrorOr<isize>     SetResGid(gid_t rgid, gid_t egid, gid_t sgid);

    ErrorOr<pid_t>     GetPGid(pid_t pid);
    ErrorOr<pid_t>     GetSid(pid_t pid);
    ErrorOr<ThreadID>  GetTid();

    ErrorOr<isize>     Futex(u32* uaddr, isize op, u32 expected,
                             const struct timespec* utime, u32* uaddr2, u32 value2);
    ErrorOr<isize>     Clone3(clone_args* uargs, usize size);
    ErrorOr<isize>     FutexWaitV(struct futex_waitv* waiters, usize futexCount,
                                  usize flags, struct timespec* timeout,
                                  clockid_t clockid);
    ErrorOr<usize> FutexWake(void* uaddr, usize mask, isize count, usize flags);
    ErrorOr<usize> FutexWait(void* uaddr, usize value, usize mask, usize flags,
                             struct timespec* timeout, clockid_t clockid);
    ErrorOr<isize> FutexRequeue(struct futex_waitv* waiters, usize flags,
                                isize wakeCount, isize requeueCount);
} // namespace API::Process
