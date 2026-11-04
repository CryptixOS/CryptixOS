/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/resource.h>
#include <API/Posix/time.h>

constexpr usize WNOHANG    = 0x00000001;
constexpr usize WUNTRACED  = 0x00000002;
constexpr usize WSTOPPED   = WUNTRACED;
constexpr usize WEXITED    = 0x00000004;
constexpr usize WCONTINUED = 0x00000008;
constexpr usize WNOWAIT    = 0x01000000; /* Don't reap, just poll status.  */

constexpr auto  WEXITSTATUS(auto x) { return (x & 0xff00) >> 8; }
constexpr auto  WTERMSIG(auto x) { return x & 0x7F; }
constexpr auto  WSTOPSIG(auto x) { return WEXITSTATUS(x); }
constexpr auto  WIFEXITED(auto x) { return WTERMSIG(x) == 0; }
constexpr auto  WIFSIGNALED(auto x)
{
    return (static_cast<i8>((x & 0x7F) + 1) >> 1) > 0;
}
constexpr auto WIFSTOPPED(auto x) { return (x & 0xFF) == 0x7F; }
constexpr auto WIFCONTINUED(auto x) { return x == 0xFFFF; }
constexpr auto W_EXITCODE(auto ret, auto sig) { return (ret << 8) | sig; }

