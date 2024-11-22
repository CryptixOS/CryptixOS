/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Logger.hpp"
#include "Utility/Types.hpp"

#include <format>
#include <string_view>

inline constexpr u64 BIT(u64 n) { return (1ull << n); }

#define CTOS_UNUSED               [[maybe_unused]]
#define CTOS_ASSERT_NOT_REACHED() __builtin_unreachable()
#define CTOS_GET_FRAME_ADDRESS(n) __builtin_frame_address(n)

#define CTOS_ARCH_X86_64          0
#define CTOS_ARCH_AARCH64         1
#define CTOS_ARCH_RISC_V          2

[[noreturn]]
inline void panic(std::string_view msg)
{
    // TODO(v1tr10l7): panic
#if CTOS_ARCH == CTOS_ARCH_X86_64
    __asm__ volatile("cli;hlt");
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    __asm__ volatile("wfi");
#endif
    CTOS_ASSERT_NOT_REACHED();
}

#define Panic(...)   panic(std::format(__VA_ARGS__))

#define Assert(expr) AssertMsg(expr, #expr)
#define AssertMsg(expr, msg)                                                   \
    !(expr) ? Panic("Assertion Failed: {}, In File: {}, At Line: {}", msg,     \
                    __FILE__, __LINE__)                                        \
            : (void)0
#define ToDo() AssertMsg(false, "Function is not implemented!")
