/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr isize SIGHUP    = 1;
constexpr isize SIGINT    = 2;
constexpr isize SIGQUIT   = 3;
constexpr isize SIGILL    = 4;
constexpr isize SIGTRAP   = 5;
constexpr isize SIGABRT   = 6;
constexpr isize SIGIOT    = 6;
constexpr isize SIGBUS    = 7;
constexpr isize SIGFPE    = 8;
constexpr isize SIGKILL   = 9;
constexpr isize SIGUSR1   = 10;
constexpr isize SIGSEGV   = 11;
constexpr isize SIGUSR2   = 12;
constexpr isize SIGPIPE   = 13;
constexpr isize SIGALRM   = 14;
constexpr isize SIGTERM   = 15;
constexpr isize SIGSTKFLT = 16;
constexpr isize SIGCHLD   = 17;
constexpr isize SIGCONT   = 18;
constexpr isize SIGSTOP   = 19;
constexpr isize SIGTSTP   = 20;
constexpr isize SIGTTIN   = 21;
constexpr isize SIGTTOU   = 22;
constexpr isize SIGURG    = 23;
constexpr isize SIGXCPU   = 24;
constexpr isize SIGXFSZ   = 25;
constexpr isize SIGVTALRM = 26;
constexpr isize SIGPROF   = 27;
constexpr isize SIGWINCH  = 28;
constexpr isize SIGIO     = 29;
constexpr isize SIGINFO   = 30;
constexpr isize SIGPOLL   = SIGIO;
constexpr isize SIGSYS    = 31;
constexpr isize SIGRTMIN  = 32;
constexpr isize _NSIG     = 64;
constexpr isize SIGRTMAX  = _NSIG;

struct sigset_t
{
    unsigned long sig[1024 / (8 * sizeof(long))];
};

// Block signals.
constexpr usize SIG_BLOCK   = 0;
// Unblock signals.
constexpr usize SIG_UNBLOCK = 1;
// Set the set of blocked signals.
constexpr usize SIG_SETMASK = 2;

using __signalfn_t          = void (*)(int);
using __sighandler_t        = __signalfn_t;
using __sigrestore_t        = void (*)(void);

struct sigaction
{
    __sighandler_t sa_handler;
    unsigned long  sa_flags;
    __sigrestore_t sa_restorer;
    sigset_t       sa_mask;
};

struct sigaltstack
{
    void* ss_sp;
    int   ss_flags;
    usize ss_size;
};
using stack_t                 = sigaltstack;

/* Error return.  */
inline __sighandler_t SIG_ERR = reinterpret_cast<__sighandler_t>(-1);
/* Default action.  */
inline __sighandler_t SIG_DFL = reinterpret_cast<__sighandler_t>(0);
/* Ignore signal.  */
inline __sighandler_t SIG_IGN = reinterpret_cast<__sighandler_t>(1);
