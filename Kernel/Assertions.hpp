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

#define EarlyPanic(fmt, ...) earlyPanic("Error Message: " fmt, __VA_ARGS__)
#define Panic(...)           panic(std::format(__VA_ARGS__).data())

#define Assert(expr)         AssertMsg(expr, #expr)
#define AssertMsg(expr, msg)                                                   \
    !(expr) ? Panic("Assertion Failed: {}, In File: {}, At Line: {}", msg,     \
                    __FILE__, __LINE__)                                        \
            : (void)0
#define ToDo()            AssertMsg(false, "Function is not implemented!")

#define AssertPMM_Ready() Assert(PMM::IsInitialized())
