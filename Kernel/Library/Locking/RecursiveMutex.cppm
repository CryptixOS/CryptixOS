/*
 * Created by v1tr10l7 on 24.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Library.Locking.RecursiveMutex;

export import :Library.Locking.Spinlock;
export import :Scheduler.Event;

export class RecursiveMutex : public NonCopyable<RecursiveMutex>
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
