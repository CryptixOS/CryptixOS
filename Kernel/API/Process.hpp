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
namespace API::Process
{
    ErrorOr<isize>  SigProcMask(i32 how, const sigset_t* newSet,
                                sigset_t* oldSet);
    ErrorOr<isize>  SchedYield();
    ErrorOr<isize>  NanoSleep(const timespec* duration, timespec* rem);

    ErrorOr<pid_t>  Pid();
    ErrorOr<pid_t>  Fork();
    ErrorOr<isize>  Execve(char* pathname, char** argv, char** envp);
    ErrorOr<isize>  Exit(isize exitcode);
    ErrorOr<isize>  Wait4(pid_t pid, isize* wstatus, isize flags,
                          rusage* rusage);

    ErrorOr<mode_t> Umask(mode_t mask);

    ErrorOr<uid_t>  GetUid();
    ErrorOr<gid_t>  GetGid();
    ErrorOr<isize>  SetUid(uid_t uid);
    ErrorOr<isize>  SetGid(gid_t gid);

    ErrorOr<uid_t>  GetEUid();
    ErrorOr<gid_t>  GetEGid();

    ErrorOr<isize>  SetPGid(pid_t pid, pid_t pgid);
    ErrorOr<pid_t>  GetPPid();

    ErrorOr<pid_t>  GetPGrp();
    ErrorOr<pid_t>  SetSid();
    ErrorOr<pid_t>  PGid();
    ErrorOr<pid_t>  Sid(pid_t pid);
} // namespace API::Process

namespace Syscall::Process
{
    ErrorOr<i32>   SysKill(Syscall::Arguments& args);

    ErrorOr<pid_t> SysSet_pGid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGetPgrp(Syscall::Arguments& args);
    ErrorOr<pid_t> SysSetSid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysGet_pGid(Syscall::Arguments& args);
    ErrorOr<pid_t> SysSid(Syscall::Arguments& args);
}; // namespace Syscall::Process
