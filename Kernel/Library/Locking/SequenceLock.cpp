/*
 * Created by v1tr10l7 on 13.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Locking/SequenceLock.hpp>

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
