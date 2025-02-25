/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <format>
#include <string_view>

#if PRISM_TARGET_CRYPTIX == false
namespace Prism
{
#endif
    extern void earlyPanic(std::string_view msg);
    extern void panic(std::string_view msg);

#if PRISM_TARGET_CRYPTIX == false
}; // namespace Prism
#endif

#define PrismAssert(expr) PrismAssertMsg(expr, #expr)
#define PrismAssertMsg(expr, msg)                                              \
    !(expr) ? PrismPanic("{}[{}]: Assertion Failed =>\n{}", __FILE__,          \
                         __LINE__, msg)                                        \
            : (void)0

#define PrismAssertFmt(expr, fmt, ...)                                         \
    !(expr) ? PrismPanic("{}[{}]: Assertion Failed =>\n{}", __FILE__,          \
                         __LINE__, std::format(fmt, __VA_ARGS__))              \
            : (void)0

#define PrismNotReached() __builtin_unreachable();

#define PrismEarlyPanic(fmt, ...)                                              \
    earlyPanic("Error Message: " fmt __VA_OPT__(, ) __VA_ARGS__)
#define PrismPanic(...) panic(std::format(__VA_ARGS__).data())

#define PrismToDo()                                                            \
    PrismAssertFmt(false, "{} is not implemented!", __PRETTY_FUNCTION__)
#define PrismToDoWarn()                                                        \
    PrismLogWarn("{}[{}]: {} is not implemented!", __FILE__, __LINE__,         \
                 __PRETTY_FUNCTION__)

#define PrismWarnAssert(cond, msg)                                             \
    {                                                                          \
        if ((cond)) PrismLogWarn(msg);                                         \
    }

#if PRISM_PREFIXLESS_MACROS == 1
    #define Assert(expr)         PrismAssert(expr)
    #define AssertMsg(expr, msg) PrismAssertMsg(expr, msg)
#endif
