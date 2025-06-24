/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Mutex.hpp>

class ConditionalVariable : public NonCopyable<ConditionalVariable>
{
  public:
    void Wait(Mutex& mutex);

    void NotifyOne();
    void NotifyAll();

  private:
    Spinlock m_Lock;
    Event    m_Event;
};
