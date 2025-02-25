/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/Arch.hpp>

#include <Assertions.hpp>
#include <Library/Logger.hpp>
#include <Library/Stacktrace.hpp>
#include <Prism/Types.hpp>

#include <format>
#include <string_view>

inline constexpr u64 BIT(u64 n) { return (1ull << n); }

#define CTOS_UNUSED               [[maybe_unused]]
#define CtosUnused(var)           ((void)var)
#define CTOS_GET_FRAME_ADDRESS(n) __builtin_frame_address(n)

#define CTOS_ARCH_X86_64          0
#define CTOS_ARCH_AARCH64         1
#define CTOS_ARCH_RISC_V          2

#define CTOS_DEBUG_SYSCALLS       0
#if CTOS_DEBUG_SYSCALLS == 1
    #define DebugSyscall(...) LogDebug(__VA_ARGS__)
#else
    #define DebugSyscall(...)
#endif

namespace CPU
{
    void SetInterruptFlag(bool);
}

CTOS_NO_KASAN void logProcessState();

CTOS_NO_KASAN [[noreturn]]
void panic(std::string_view msg);
CTOS_NO_KASAN [[noreturn]]
void earlyPanic(const char* format, ...);
