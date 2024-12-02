/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <errno.h>

#include "Arch/CPU.hpp"
#include "Scheduler/Thread.hpp"

static errno_t      error;

extern "C" errno_t* __errno_location()
{
    Thread* thread = CPU::GetCurrentThread();
    return reinterpret_cast<errno_t*>(thread ? &thread->error : &error);
}
