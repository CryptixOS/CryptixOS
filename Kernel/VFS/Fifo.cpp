/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/String/StringUtils.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <Prism/Containers/RingBuffer.hpp>

module Cryptix:Impl.VFS.Fifo;

import :Scheduler.Process;
import :VFS.Fifo;

constexpr usize FIFO_SIZE = 16 * PMM::PAGE_SIZE;

Pipe            Fifo::CreatePipe()
{
    auto fifo   = new Fifo();
    auto dentry = CreateRef<DirectoryEntry>("");

    Pipe pipe{};
    // FIXME(v1tr10l7): Clean up fifo
    pipe.Reader
        = CreateRef<FileDescriptor>(dentry, fifo, 0, FileAccessMode::eRead);
    ++fifo->m_ReaderCount;

    pipe.Writer
        = CreateRef<FileDescriptor>(dentry, fifo, 0, FileAccessMode::eWrite);
    ++fifo->m_WriterCount;

    return pipe;
}

Fifo::Fifo() { m_Buffer.Reserve(FIFO_SIZE); }

::Ref<FileDescriptor> Fifo::OpenDirection(Fifo::Direction direction)
{
    auto dentry = CreateRef<DirectoryEntry>("");
    auto fd     = CreateRef<FileDescriptor>(dentry, this, 0,
                                        direction == Direction::eRead
                                                ? FileAccessMode::eRead
                                                : FileAccessMode::eWrite);
    if (direction == Direction::eRead) ++m_ReaderCount;
    else ++m_WriterCount;

    LogTrace("Fifo: Opening for {}", StringUtils::ToString(direction));
    m_Event.Trigger(false);
    return fd;
}
void Fifo::Close(bool writer)
{
    if (writer) --m_WriterCount;
    else --m_ReaderCount;

    m_Event.Trigger(false);
}

ErrorOr<isize> Fifo::Read(void* buffer, off_t offset, usize count)
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

    count = Min(count, m_Buffer.Used());
    nread = AsUser(
        [&]() -> isize
        { return m_Buffer.Read(reinterpret_cast<u8*>(buffer), count); });

    m_Event.Trigger(false);
cleanup:
    m_Lock.Release();
    return nread;
}
ErrorOr<isize> Fifo::Write(const void* buffer, off_t offset, usize count)
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

    nwritten = AsUser(
        [&]() -> isize
        { return m_Buffer.Write(reinterpret_cast<const u8*>(buffer), count); });
    m_Event.Trigger();

cleanup:
    m_Lock.Release();
    return nwritten;
}

ErrorOr<const stat> Fifo::Stat() const
{
    stat stats{};
    stats.st_mode = S_IFIFO;

    return stats;
}
