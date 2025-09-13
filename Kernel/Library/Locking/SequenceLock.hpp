/*
 * Created by v1tr10l7 on 13.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>

class SequenceLock
{
  public:
    inline SequenceLock() = default;

    void  WriteBegin(bool saveIrqs = false);
    void  WriteEnd(bool restoreIrqs = false);

    void  WriteBeginIrqSave();
    void  WriteEndIrqRestore();

    usize ReadBegin() const;
    bool  ReadRetry(usize start) const;

  private:
    Spinlock      m_Lock;
    Atomic<usize> m_Sequence = 0;
};

template <typename T>
class SequenceProtected : public NonCopyable<SequenceProtected<T>>,
                          public NonMovable<SequenceProtected<T>>
{
  public:
    inline SequenceProtected() = default;

    template <typename F>
    void Write(F&& fn)
    {
        m_SequenceLock.WriteBegin();
        fn(m_Data);
        m_SequenceLock.WriteEnd();
    }
    template <typename F>
    inline auto Read(F&& fn) -> decltype(fn(DeclVal<const T&>()))
    {
        usize                             sequence;
        decltype(fn(DeclVal<const T&>())) result;

        do {
            sequence = m_SequenceLock.ReadBegin();
            result   = fn(m_Data);
        } while (m_SequenceLock.ReadRetry(sequence));
        return result;
    }

  private:
    mutable SequenceLock m_SequenceLock;
    T                    m_Data{};
};

class SequenceWriterGuard final : public NonCopyable<SequenceWriterGuard>
{
  public:
    SequenceWriterGuard()                                 = delete;
    SequenceWriterGuard& operator=(SequenceWriterGuard&&) = delete;

    explicit SequenceWriterGuard(SequenceLock& lock,
                                 bool          disableInterrupts = false)
        : m_SequenceLock(lock)
    {
        m_SequenceLock.WriteBegin();
    }
    ~SequenceWriterGuard() { m_SequenceLock.WriteEnd(); }

  private:
    SequenceLock& m_SequenceLock;
};
