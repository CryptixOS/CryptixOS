/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>
#include <Scheduler/Event.hpp>

class RecursiveMutex : public NonCopyable<RecursiveMutex>
{
  public:
    RecursiveMutex() = default;

    void Lock();
    bool TryLock();
    void Unlock();

  private:
    Spinlock m_Lock;
    bool     m_Locked = false;
    usize    m_Depth  = 0;
    Thread*  m_Owner  = nullptr;
    Event    m_Event;
};
