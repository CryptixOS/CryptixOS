/*
 * Created by v1tr10l7 on 13.09.2025.
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
#include <Prism/Core/TypeTraits.hpp>

module Cryptix:Impl.Library.Locking.SequenceLock;

import :Library.Locking.SequenceLock;

void SequenceLock::WriteBegin(bool saveIrqs)
{
    m_Lock.Acquire(saveIrqs);
    m_Sequence.FetchAdd(1, MemoryOrder::eRelaxed);
    AtomicThreadFence(MemoryOrder::eRelease);
}
void SequenceLock::WriteEnd(bool restoreIrqs)
{
    AtomicThreadFence(MemoryOrder::eRelease);
    m_Sequence.FetchAdd(1, MemoryOrder::eRelaxed);
    m_Lock.Release(restoreIrqs);
}
void  SequenceLock::WriteBeginIrqSave() { return WriteBegin(true); }
void  SequenceLock::WriteEndIrqRestore() { return WriteEnd(true); }

usize SequenceLock::ReadBegin() const
{
    usize sequence = 0;
    do {
        sequence = m_Sequence.Load(MemoryOrder::eAcquire);
    } while ((sequence & 1) != 0);

    return sequence;
}
bool SequenceLock::ReadRetry(usize start) const
{
    AtomicThreadFence(MemoryOrder::eAcquire);
    return m_Sequence.Load(MemoryOrder::eRelaxed) != start;
}
