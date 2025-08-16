/*
 * Created by v1tr10l7 on 16.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Mutex.hpp>

class UniqueGuard final : public NonCopyable<UniqueGuard>
{
  public:
    UniqueGuard()                         = delete;
    UniqueGuard& operator=(UniqueGuard&&) = delete;

    explicit UniqueGuard(Mutex& lock)
        : m_Lock(lock)
    {
        lock.Lock();
    }
    ~UniqueGuard() { m_Lock.Unlock(); }

  private:
    Mutex& m_Lock;
};
