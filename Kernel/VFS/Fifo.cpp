/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Process.hpp>

#include <VFS/Fifo.hpp>

constexpr usize FIFO_SIZE = 16 * PMM::PAGE_SIZE;

Fifo::Fifo()
    : INode("Fifo")
{
    m_Stats.st_mode = 0644 | S_IFIFO;
    m_Data          = new u8[FIFO_SIZE];
    m_Capacity      = FIFO_SIZE;
}

FileDescriptor* Fifo::Open(Fifo::Direction direction)
{

    auto fd = new FileDescriptor(this, 0,
                                 direction == Direction::eRead
                                     ? FileAccessMode::eRead
                                     : FileAccessMode::eWrite);
    if (direction == Direction::eRead) ++m_ReaderCount;
    else ++m_WriterCount;
    return fd;
}

isize Fifo::Read(void* buffer, off_t offset, usize bytes)
{
    isize nread      = 0;
    usize beforeWrap = 0;
    usize afterWrap  = 0;
    usize newOffset  = 0;
    m_Lock.Acquire();

    while (m_Size == 0)
    {
        if (m_WriterCount == 0 || m_NonBlocking) goto cleanup;
        m_Lock.Release();
        m_Event.Await();

        m_Lock.Acquire();
    }

    if (m_Size < bytes) bytes = m_Size;

    if (m_ReadOffset + bytes > m_Capacity)
    {
        beforeWrap = m_Capacity - m_ReadOffset;
        afterWrap  = bytes - beforeWrap;
        newOffset  = afterWrap;
    }
    else
    {
        beforeWrap = bytes;
        afterWrap  = 0;
        newOffset  = m_ReadOffset + bytes;

        if (newOffset == m_Capacity) newOffset = 0;
    }

    std::memcpy(buffer, m_Data + m_ReadOffset, beforeWrap);
    if (afterWrap != 0)
        std::memcpy(reinterpret_cast<u8*>(buffer) + beforeWrap, m_Data,
                    afterWrap);

    m_ReadOffset = newOffset;
    m_Size -= bytes;

    m_Event.Trigger(false);
    nread = bytes;

cleanup:
    m_Lock.Release();
    return nread;
}
isize Fifo::Write(const void* buffer, off_t offset, usize bytes)
{
    isize written    = 0;
    usize beforeWrap = 0;
    usize afterWrap  = 0;
    usize newOffset  = 0;
    m_Lock.Acquire();

    if (m_ReaderCount == 0)
    {
        errno = EPIPE;
        goto cleanup;
    }

    while (m_Size == m_Capacity)
    {
        m_Lock.Release();
        m_Event.Await(true);
        m_Lock.Acquire();
    }

    if (m_Size + bytes > m_Capacity) bytes = m_Capacity - m_Size;

    if (m_WriteOffset + bytes > m_Capacity)
    {
        beforeWrap = m_Capacity - m_WriteOffset;
        afterWrap  = bytes - beforeWrap;
        newOffset  = afterWrap;
    }
    else
    {
        beforeWrap = bytes;
        afterWrap  = 0;
        newOffset  = m_WriteOffset + bytes;

        if (newOffset == m_Capacity) newOffset = 0;
    }

    std::memcpy(m_Data + m_WriteOffset, buffer, beforeWrap);
    if (afterWrap != 0)
        std::memcpy(m_Data, reinterpret_cast<const u8*>(buffer) + beforeWrap,
                    afterWrap);

    m_WriteOffset = newOffset;
    m_Size += bytes;

    m_Event.Trigger();
    written = bytes;

cleanup:
    m_Lock.Release();
    return written;
}
