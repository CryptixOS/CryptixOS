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

// Data written while the kernel initializes and never again: the section is
// remapped read-only by MM::FinishInitialization(), so a stray write later
// faults instead of corrupting it (Linux's __ro_after_init). Only for
// variables that are constant-initialized or set during boot.
#define CTOS_RO_AFTER_INIT [[gnu::section(".ro_after_init")]]

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
