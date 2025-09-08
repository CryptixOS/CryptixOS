/*
 * Created by v1tr10l7 on 08.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/signal.h>

#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>

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
enum class SignalFlags
{
    eNone       = 0x00,
    eUnkillable = Bit(7),
};

constexpr SignalFlags operator~(SignalFlags lhs)
{
    return static_cast<SignalFlags>(~ToUnderlying(lhs));
}
constexpr SignalFlags operator|(SignalFlags lhs, SignalFlags rhs)
{
    auto result = ToUnderlying(lhs) | ToUnderlying(rhs);

    return static_cast<SignalFlags>(result);
}
constexpr bool operator&(SignalFlags& lhs, SignalFlags rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}
constexpr SignalFlags& operator|=(SignalFlags& lhs, SignalFlags rhs)
{
    auto result = ToUnderlying(lhs) | ToUnderlying(rhs);

    return lhs  = static_cast<SignalFlags>(result), lhs;
}
constexpr SignalFlags& operator&=(SignalFlags& lhs, SignalFlags rhs)
{
    auto result = ToUnderlying(lhs) & ToUnderlying(rhs);

    return lhs  = static_cast<SignalFlags>(result), lhs;
}
