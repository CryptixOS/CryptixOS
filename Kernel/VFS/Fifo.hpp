/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/RingBuffer.hpp>
#include <Scheduler/Event.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>

struct Pipe
{
    ::Ref<FileDescriptor> Reader;
    ::Ref<FileDescriptor> Writer;
};
class Fifo : public File
{
  public:
    static Pipe CreatePipe();

    Fifo();

    enum class Direction
    {
        eRead  = 0,
        eWrite = 1,
    };

    ::Ref<FileDescriptor>  OpenDirection(Direction direction);
    virtual void           Close(bool writer) override;

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;
    virtual ErrorOr<const stat> Stat() const override;
    virtual bool                IsFifo() const override { return true; }

  private:
    Spinlock      m_Lock;
    Atomic<usize> m_ReaderCount = 0;
    Atomic<usize> m_WriterCount = 0;
    Event         m_Event;
    RingBuffer    m_Buffer;

    bool          m_NonBlocking = false;

    void          Attach(Direction direction);
    void          Detach();
};
