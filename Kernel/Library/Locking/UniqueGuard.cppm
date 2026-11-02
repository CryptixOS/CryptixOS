/*
 * Created by v1tr10l7 on 16.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Library.Locking.UniqueGuard;

export import :Library.Locking.Mutex;

export class UniqueGuard final : public NonCopyable<UniqueGuard>
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
