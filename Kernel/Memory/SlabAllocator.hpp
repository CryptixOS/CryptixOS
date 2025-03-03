/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Spinlock.hpp>
#include <Prism/Math.hpp>
#include <Prism/Core/Types.hpp>

class SlabAllocatorBase
{
  public:
    virtual void* Allocate()          = 0;
    virtual void  Free(void* memory)  = 0;
    virtual usize GetAllocationSize() = 0;
};

struct SlabHeader
{
    SlabAllocatorBase* Slab;
};

template <usize Bytes>
class SlabAllocator : public SlabAllocatorBase
{
  public:
    SlabAllocator() = default;
    void Initialize()
    {
        m_FirstFree = ToHigherHalfAddress<uintptr_t>(
            PhysicalMemoryManager::CallocatePages(1));
        auto available
            = 0x1000 - Math::AlignUp(sizeof(SlabHeader), m_AllocationSize);
        auto slabPointer  = reinterpret_cast<SlabHeader*>(m_FirstFree);
        slabPointer->Slab = this;
        m_FirstFree += Math::AlignUp(sizeof(SlabHeader), m_AllocationSize);

        auto array = reinterpret_cast<usize*>(m_FirstFree);
        auto max   = available / m_AllocationSize - 1;
        auto fact  = m_AllocationSize / 8;
        for (usize i = 0; i < max; i++)
            array[i * fact] = reinterpret_cast<usize>(&array[(i + 1) * fact]);
        array[max * fact] = 0;
    }

    void* Allocate() override
    {
        ScopedLock guard(m_Lock);
        if (!m_FirstFree) Initialize();

        auto oldFree = reinterpret_cast<usize*>(m_FirstFree);
        m_FirstFree  = oldFree[0];
        std::memset(oldFree, 0, m_AllocationSize);

        return oldFree;
    }
    void Free(void* memory) override
    {
        ScopedLock guard(m_Lock);
        if (!memory) return;

        auto newHead = static_cast<usize*>(memory);
        newHead[0]   = m_FirstFree;
        m_FirstFree  = reinterpret_cast<uintptr_t>(newHead);
    }

    virtual usize GetAllocationSize() override { return m_AllocationSize; }

    template <typename T>
    T Allocate()
    {
        return reinterpret_cast<T>(Allocate());
    }
    template <typename T>
    void Free(T memory)
    {
        return Free(reinterpret_cast<void*>(memory));
    }

  private:
    uintptr_t m_FirstFree      = 0;
    usize     m_AllocationSize = Bytes;
    Spinlock  m_Lock;
};
