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

export module Cryptix:Library.Locking.Semaphore;

export import :Library.Locking.Spinlock;
export import :Scheduler.Event;

export class Semaphore : public NonCopyable<Semaphore>
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
