/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

constexpr usize SIGHUP      = 1;
constexpr usize SIGINT      = 2;
constexpr usize SIGQUIT     = 3;
constexpr usize SIGILL      = 4;
constexpr usize SIGTRAP     = 5;
constexpr usize SIGABRT     = 6;
constexpr usize SIGIOT      = 6;
constexpr usize SIGBUS      = 7;
constexpr usize SIGFPE      = 8;
constexpr usize SIGKILL     = 9;
constexpr usize SIGUSR1     = 10;
constexpr usize SIGSEGV     = 11;
constexpr usize SIGUSR2     = 12;
constexpr usize SIGPIPE     = 13;
constexpr usize SIGALRM     = 14;
constexpr usize SIGTERM     = 15;
constexpr usize SIGSTKFLT   = 16;
constexpr usize SIGCHLD     = 17;
constexpr usize SIGCONT     = 18;
constexpr usize SIGSTOP     = 19;
constexpr usize SIGTSTP     = 20;
constexpr usize SIGTTIN     = 21;
constexpr usize SIGTTOU     = 22;
constexpr usize SIGURG      = 23;
constexpr usize SIGXCPU     = 24;
constexpr usize SIGXFSZ     = 25;
constexpr usize SIGVTALRM   = 26;
constexpr usize SIGPROF     = 27;
constexpr usize SIGWINCH    = 28;
constexpr usize SIGIO       = 29;
constexpr usize SIGINFO     = 30;
constexpr usize SIGPOLL     = SIGIO;

using sigset_t              = usize;

// Block signals.
constexpr usize SIG_BLOCK   = 0;
// Unblock signals.
constexpr usize SIG_UNBLOCK = 1;
// Set the set of blocked signals.
constexpr usize SIG_SETMASK = 2;
