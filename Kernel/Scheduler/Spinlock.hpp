/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/Arch.hpp"

class Spinlock
{
  public:
    bool TestAndAcquire() { return __sync_bool_compare_and_swap(&lock, 0, 1); }

    void Acquire()
    {
        volatile usize deadLockCounter = 0;
        for (;;)
        {
            if (TestAndAcquire()) break;
            deadLockCounter += 1;
            if (deadLockCounter >= 100000000) goto deadlock;

            Arch::Pause();
        }

        lastAcquirer = __builtin_return_address(0);
        return;

    deadlock:
        Panic("DEADLOCK");
    }
    void Release()
    {
        lastAcquirer = nullptr;
        __sync_bool_compare_and_swap(&lock, 1, 0);
    }

  private:
    i32   lock         = 0;
    void* lastAcquirer = nullptr;
};
