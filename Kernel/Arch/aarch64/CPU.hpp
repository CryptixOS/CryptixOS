/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

struct Thread;
namespace CPU
{
    struct CPU
    {
        usize     ID;
        void*     Empty;

        uintptr_t ThreadStack;
        uintptr_t KernelStack;

        usize     HardwareID;
        bool      IsOnline = false;

        Thread*   Idle;
        Thread*   CurrentThread;

        bool      DuringSyscall = false;
        usize     LastSyscallID = usize(-1);
    };
}; // namespace CPU
