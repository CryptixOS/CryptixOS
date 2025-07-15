/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Utility/Math.hpp>

class AllocatorBase
{
  public:
    AllocatorBase()          = default;
    virtual ~AllocatorBase() = default;

    virtual ErrorOr<void> Initialize(usize chunkSize, Pointer base = nullptr, usize length = 0)
        = 0;
    virtual void    Shutdown()             = 0;

    virtual Pointer Allocate()             = 0;
    virtual void    Free(Pointer memory)   = 0;

    virtual usize   AllocationSize()       = 0;

    virtual usize   TotalAllocated() const = 0;
    virtual usize   TotalFreed() const     = 0;
    virtual usize   Used() const           = 0;
};

struct SlabFrame
{
    SlabFrame* Next = nullptr;
};
struct SlabHeader
{
    AllocatorBase* Slab = nullptr;
};

class SlabAllocator : public AllocatorBase
{
  public:
    SlabAllocator() {}

    virtual ErrorOr<void> Initialize(usize chunkSize, Pointer base   = nullptr,
                                     usize   length = 0) override
    {
        if (base) Assert(length > 0);

        Assert(Math::IsPowerOfTwo(chunkSize));
        m_ChunkSize = chunkSize;

        m_FirstFree = base ?: Pointer(PMM::CallocatePages(1)).ToHigherHalf();
        if (!m_FirstFree) return Error(ENOMEM);

        auto available
            = PMM::PAGE_SIZE - Math::AlignUp(sizeof(SlabHeader), m_ChunkSize);
        auto       slabHeader = m_FirstFree.As<SlabHeader>();

        ScopedLock guard(m_Lock);
        slabHeader->Slab = this;
        m_FirstFree += Math::AlignUp(sizeof(SlabHeader), m_ChunkSize);

        auto       count = available / m_ChunkSize;
        SlabFrame* prev  = nullptr;
        for (usize i = 0; i < count; i++)
        {
            auto frame  = m_FirstFree.Offset<SlabFrame*>(i * m_ChunkSize);
            frame->Next = prev;
            prev        = frame;
        }
        m_FirstFree = prev;
        return {};
    }
    virtual void    Shutdown() override { ToDoWarn(); }

    virtual Pointer Allocate() override
    {
        if (!m_FirstFree && !Initialize(m_ChunkSize)) return nullptr;

        ScopedLock guard(m_Lock);
        auto       frame = m_FirstFree.As<SlabFrame>();
        m_FirstFree      = frame->Next;

        m_TotalAllocated += m_ChunkSize;
        return frame;
    }
    virtual void Free(Pointer memory) override
    {
        if (!memory) return;

        ScopedLock guard(m_Lock);
        auto       frame = memory.As<SlabFrame>();
        frame->Next      = m_FirstFree;
        m_FirstFree      = frame;

        m_TotalFreed += m_ChunkSize;
        return;

        auto newHead = memory.As<usize>();
        newHead[0]   = m_FirstFree;
        m_FirstFree  = newHead;
    }

    virtual usize AllocationSize() override { return m_ChunkSize; }

    virtual usize TotalAllocated() const override { return m_TotalAllocated; }
    virtual usize TotalFreed() const override { return m_TotalFreed; }
    virtual usize Used() const override
    {
        return TotalAllocated() - TotalFreed();
    }

  private:
    Spinlock m_Lock;
    Pointer  m_FirstFree;
    usize    m_ChunkSize;

    usize    m_TotalAllocated;
    usize    m_TotalFreed;
};
