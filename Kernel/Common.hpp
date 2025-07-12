/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Library/Stacktrace.hpp>
#include <Prism/Core/Types.hpp>

#define KERNEL_INIT_CODE_SECTION_NAME ".kernel_init"
#define KERNEL_INIT_CODE                                                       \
    [[gnu::section(KERNEL_INIT_CODE_SECTION_NAME), gnu::used]]

#define CTOS_ALWAYS_INLINE     [[gnu::always_inline]]
#define CTOS_UNUSED            [[maybe_unused]]

#define CtosUnused(var)        ((void)var)
#define CtosGetFrameAddress(n) __builtin_frame_address(n)

#define CTOS_ARCH_X86_64       0
#define CTOS_ARCH_AARCH64      1
#define CTOS_ARCH_RISC_V       2

#define CTOS_DEBUG_SYSCALLS    0
#if CTOS_DEBUG_SYSCALLS == 1
    #define DebugSyscall(...) LogDebug(__VA_ARGS__)
#else
    #define DebugSyscall(...)
#endif

#define RetOnError(...)                                                        \
    {                                                                          \
        auto result = (__VA_ARGS__);                                           \
        if (!result) return Error(result.error());                             \
    }
