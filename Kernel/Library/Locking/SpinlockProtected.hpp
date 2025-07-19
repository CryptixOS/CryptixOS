/*
 * Created by v1tr10l7 on 17.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>

#include <Library/Locking/RecursiveSpinlock.hpp>

template <typename T>
concept SpinlockLocker = requires { IsSameV<T, ScopedLock>; };
template <typename T>
class SpinlockProtected : public NonCopyable<SpinlockProtected<T>>,
                          public NonMovable<SpinlockProtected<T>>
{
  public:
    template <typename... Args>
    SpinlockProtected(Args&&... args)
        : m_Value(Forward<Args>(args)...)
    {
    }

    template <typename Callback>
    decltype(auto) With(Callback callback) const
    {
        auto guard = LockRead();
        return callback(*guard);
    }

    template <typename Callback>
    decltype(auto) With(Callback callback)
    {
        auto guard = LockWrite();
        return callback(*guard);
    }

    template <typename Callback>
    void ForEachRead(Callback callback) const
    {
        With(
            [&](auto const& value)
            {
                for (auto it = value.begin(); it != value.end(); it++)
                    callback(*it);
            });
    }

    template <typename Callback>
    void ForEach(Callback callback)
    {
        With(
            [&](auto& value)
            {
                for (auto it = value.begin(); it != value.end(); it++)
                    callback(*it);
            });
    }

  private:
    template <typename U>
    class Locked : public NonCopyable<Locked<U>>, public NonMovable<Locked<U>>
    {
      public:
        Locked(U& value, RecursiveSpinlock& lock)
            : m_Value(value)
            , m_Lock(lock)
        {
        }

        CTOS_ALWAYS_INLINE U const* operator->() const { return &m_Value; }
        CTOS_ALWAYS_INLINE U const& operator*() const { return m_Value; }

        CTOS_ALWAYS_INLINE U*       operator->() { return &m_Value; }
        CTOS_ALWAYS_INLINE U&       operator*() { return m_Value; }

        CTOS_ALWAYS_INLINE U const& Acquire() const { return m_Value; }
        CTOS_ALWAYS_INLINE U&       Acquire() { return m_Value; }

      private:
        U&                            m_Value;
        ScopedRecursiveLock m_Lock;
    };

    auto LockRead() const { return Locked<T const>(m_Value, m_Lock); }
    auto LockWrite() { return Locked<T>(m_Value, m_Lock); }

    T    m_Value;
    RecursiveSpinlock mutable m_Lock;
};
