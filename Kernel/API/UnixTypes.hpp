/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/signal.h>
#include <API/Posix/sys/stat.h>

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

using ssize_t     = isize;

using pid_t       = int;
using tid_t       = int;

using ProcessID   = pid_t;
using ThreadID    = tid_t;

using DeviceID    = dev_t;
using DeviceMajor = u32;
using DeviceMinor = u32;

using INodeMode   = mode_t;
using LinkCount   = nlink_t;
using UserID      = uid_t;
using GroupID     = gid_t;

enum class SignalID
{
    eHangup                = SIGHUP,
    eInterrupt             = SIGINT,
    eQuit                  = SIGQUIT,
    eIllegal               = SIGILL,
    eTrap                  = SIGTRAP,
    eAbort                 = SIGABRT,
    eIotTrap               = SIGIOT,
    eBusError              = SIGBUS,
    eArithmeticError       = SIGFPE,
    eKill                  = SIGKILL,
    eUser1                 = SIGUSR1,
    eSegmentationFault     = SIGSEGV,
    eUser2                 = SIGUSR2,
    ePipe                  = SIGPIPE,
    eAlarm                 = SIGALRM,
    eTerminate             = SIGTERM,
    eStackFault            = SIGSTKFLT,
    eChild                 = SIGCHLD,
    eContinue              = SIGCONT,
    eStop                  = SIGSTOP,
    eStopTerminal          = SIGTSTP,
    eTerminalInput         = SIGTTIN,
    eTerminalOutput        = SIGTTOU,
    eUrgent                = SIGURG,
    eCPUTimeExceeded       = SIGXCPU,
    eFileSizelimitExceeded = SIGXFSZ,
    eVirtualAlarmClock     = SIGVTALRM,
    eProfilerTimerFired    = SIGPROF,
    eWinch                 = SIGWINCH,
    eIO                    = SIGIO,
    eInfo                  = SIGINFO,
    ePoll                  = eInfo,
    eBadSystemCall         = SIGSYS,

    eFirstRealTime         = SIGRTMIN,
    eLastRealTime          = SIGRTMAX,
    eCount                 = _NSIG,
};
struct SignalAction
{
    Pointer VirtualAddress = nullptr;
    isize   Flags          = 0;
    usize   Mask           = 0;
};
