/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>
#include <Scheduler/Event.hpp>

class Semaphore : public NonCopyable<Semaphore>
{
  public:
    explicit Semaphore(usize initial = 0)
        : m_Count(initial)
    {
    }

    void Signal();
    void Wait();
    bool TryWait();

  private:
    Spinlock m_Lock;
    usize    m_Count = 0;
    Event    m_Event;
};
