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
    eCount                 = _NSIG + 1,
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

struct SignalSet
{
    static constexpr usize BitCount     = 1024;
    static constexpr usize WordBitCount = sizeof(unsigned long) * 8;
    static constexpr usize WordCount    = BitCount / WordBitCount;
    unsigned long          Signals[WordCount];

    constexpr auto SignalIndex(isize signal) const { return signal - 1; }
    constexpr auto WordIndex(isize signal) const
    {
        return SignalIndex(signal) / WordBitCount;
    }

    constexpr auto BitMask(isize signal) const
    {
        return 1ul << (SignalIndex(signal) % WordBitCount);
    }

    inline constexpr bool operator[](usize i) const { return Contains(i); }

    inline constexpr void Clear()
    {
        for (auto& signal : Signals) signal = 0;
    }
    inline constexpr void Fill()
    {
        for (auto& signal : Signals) signal = ~0ull;
    }

    constexpr void Add(isize signal)
    {
        if (signal <= 0 || signal > static_cast<isize>(BitCount)) return;
        Signals[WordIndex(signal)] |= 1ul
                                   << (SignalIndex(signal) % WordBitCount);
    }

    constexpr void Remove(isize signal)
    {
        if (signal <= 0 || signal > static_cast<isize>(BitCount)) return;
        Signals[WordIndex(signal)]
            &= ~(1ul << (SignalIndex(signal) % WordBitCount));
    }

    constexpr bool Contains(isize signal) const
    {
        if (signal <= 0 || signal > static_cast<isize>(BitCount)) return false;
        usize         word = WordIndex(signal);
        unsigned long mask = 1ul << (SignalIndex(signal) % WordBitCount);
        return (Signals[word] & mask) != 0;
    }

    friend constexpr bool operator==(const SignalSet& lhs, const SignalSet& rhs)
    {
        for (usize i = 0; i < WordCount; ++i)
            if (lhs.Signals[i] != rhs.Signals[i]) return false;
        return true;
    }
    friend constexpr bool operator!=(const SignalSet& lhs, const SignalSet& rhs)
    {
        return !(lhs == rhs);
    }

    friend constexpr SignalSet operator~(const SignalSet& set)
    {
        SignalSet result;
        for (usize i = 0; i < WordCount; ++i)
            result.Signals[i] = ~set.Signals[i];
        return result;
    }
    friend constexpr SignalSet operator&(const SignalSet& lhs,
                                         const SignalSet& rhs)
    {
        SignalSet result;
        for (usize i = 0; i < WordCount; ++i)
            result.Signals[i] = lhs.Signals[i] & rhs.Signals[i];
        return result;
    }

    friend constexpr SignalSet operator|(const SignalSet& lhs,
                                         const SignalSet& rhs)
    {
        SignalSet result;
        for (usize i = 0; i < WordCount; ++i)
            result.Signals[i] = lhs.Signals[i] | rhs.Signals[i];
        return result;
    }

    // Compound assignment
    constexpr SignalSet& operator&=(const SignalSet& rhs)
    {
        for (usize i = 0; i < WordCount; ++i) Signals[i] &= rhs.Signals[i];
        return *this;
    }

    constexpr SignalSet& operator|=(const SignalSet& rhs)
    {
        for (usize i = 0; i < WordCount; ++i) Signals[i] |= rhs.Signals[i];
        return *this;
    }
};
