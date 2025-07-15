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

struct SlabFrame
{
    SlabFrame* Next = nullptr;
};

class SlabAllocatorBase
{
  public:
    virtual usize AllocationSize()     = 0;
    virtual void  Free(Pointer memory) = 0;
};
struct SlabHeader
{
    SlabAllocatorBase* Slab = nullptr;
};

template <typename PageAllocPolicy, typename LockPolicy>
class SlabAllocator : public SlabAllocatorBase
{
  public:
    SlabAllocator() {}

    virtual ErrorOr<void> Initialize(usize chunkSize)
    {
        Assert(Math::IsPowerOfTwo(chunkSize));
        m_ChunkSize = chunkSize;

        m_FirstFree = PageAllocPolicy::CallocatePages(1);
        if (!m_FirstFree) return Error(ENOMEM);

        auto available
            = PMM::PAGE_SIZE - Math::AlignUp(sizeof(SlabHeader), m_ChunkSize);
        auto slabHeader  = m_FirstFree.As<SlabHeader>();

        auto guard       = m_Lock.Lock();
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
    virtual void    Shutdown() { ToDoWarn(); }

    virtual Pointer Allocate()
    {
        if (!m_FirstFree && !Initialize(m_ChunkSize)) return nullptr;

        auto guard  = m_Lock.Lock();
        auto frame  = m_FirstFree.As<SlabFrame>();
        m_FirstFree = frame->Next;

        m_TotalAllocated += m_ChunkSize;
        return frame;
    }
    virtual void Free(Pointer memory) override
    {
        if (!memory) return;

        auto guard  = m_Lock.Lock();
        auto frame  = memory.As<SlabFrame>();
        frame->Next = m_FirstFree;
        m_FirstFree = frame;

        m_TotalFreed += m_ChunkSize;
        return;

        auto newHead = memory.As<usize>();
        newHead[0]   = m_FirstFree;
        m_FirstFree  = newHead;
    }

    virtual usize AllocationSize() override { return m_ChunkSize; }

    virtual usize TotalAllocated() const { return m_TotalAllocated; }
    virtual usize TotalFreed() const { return m_TotalFreed; }
    virtual usize Used() const { return TotalAllocated() - TotalFreed(); }

  private:
    LockPolicy m_Lock;
    Pointer    m_FirstFree;
    usize      m_ChunkSize;

    usize      m_TotalAllocated;
    usize      m_TotalFreed;
};
