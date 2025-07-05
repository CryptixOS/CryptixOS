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
    m_Metadata.Mode = 0644 | S_IFIFO;
    m_Buffer.Reserve(FIFO_SIZE);
}

FileDescriptor* Fifo::Open(Fifo::Direction direction)
{

    auto fd = new FileDescriptor(nullptr, 0,
                                 direction == Direction::eRead
                                     ? FileAccessMode::eRead
                                     : FileAccessMode::eWrite);
    if (direction == Direction::eRead) ++m_ReaderCount;
    else ++m_WriterCount;

    m_Event.Trigger(false);
    return fd;
}

isize Fifo::Read(void* buffer, off_t offset, usize count)
{
    isize nread = 0;
    m_Lock.Acquire();

    while (m_Buffer.Used() == 0)
    {
        if (m_WriterCount == 0 || m_NonBlocking)
        {
            nread = 0;
            goto cleanup;
        }

        m_Lock.Release();
        m_Event.Await();
        m_Lock.Acquire();
    }

    count = std::min(count, m_Buffer.Used());
    nread = CPU::AsUser(
        [&]() -> isize
        { return m_Buffer.Read(reinterpret_cast<u8*>(buffer), count); });

    m_Event.Trigger(false);
cleanup:
    m_Lock.Release();
    return nread;
}
isize Fifo::Write(const void* buffer, off_t offset, usize count)
{
    isize nwritten = 0;
    m_Lock.Acquire();
    if (m_ReaderCount == 0)
    {
        errno    = EPIPE;
        nwritten = -1;
        goto cleanup;
    }

    while (m_Buffer.Used() == m_Buffer.Capacity())
    {
        m_Lock.Release();
        m_Event.Await();
        m_Lock.Acquire();
    }

    if (m_Buffer.Used() + count > m_Buffer.Capacity())
        count = m_Buffer.Capacity() - m_Buffer.Used();

    nwritten = CPU::AsUser(
        [&]() -> isize
        { return m_Buffer.Write(reinterpret_cast<const u8*>(buffer), count); });
    m_Event.Trigger();

cleanup:
    m_Lock.Release();
    return nwritten;
}
