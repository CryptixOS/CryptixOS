/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

namespace PhysicalMemoryManager
{
    extern bool IsInitialized();
}
namespace PMM = PhysicalMemoryManager;

#define Assert(expr) AssertMsg(expr, #expr)
#define AssertMsg(expr, msg)                                                   \
    !(expr)                                                                    \
        ? Panic("{}[{}]: Assertion Failed =>\n{}", __FILE__, __LINE__, msg)    \
        : (void)0

#define AssertFmt(expr, fmt, ...)                                              \
    !(expr) ? Panic("{}[{}]: Assertion Failed =>\n{}", __FILE__, __LINE__,     \
                    std::format(fmt, __VA_ARGS__))                             \
            : (void)0

#define AssertNotReached() __builtin_unreachable();

#define EarlyPanic(fmt, ...)                                                   \
    earlyPanic("Error Message: " fmt __VA_OPT__(, ) __VA_ARGS__)
#define Panic(...) panic(std::format(__VA_ARGS__).data())

#define ToDo()     AssertFmt(false, "{} is not implemented!", __PRETTY_FUNCTION__)
#define ToDoWarn()                                                             \
    LogWarn("{}[{}]: {} is not implemented!", __FILE__, __LINE__,              \
            __PRETTY_FUNCTION__)

#define AssertPMM_Ready() Assert(PMM::IsInitialized())

#define DebugWarnIf(cond, msg)                                                 \
    {                                                                          \
        if ((cond)) LogWarn(msg);                                              \
    }
