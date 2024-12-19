/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include <Scheduler/Spinlock.hpp>
#include <Utility/Math.hpp>

class SlabAllocatorBase
{
  public:
    virtual void* Allocate()          = 0;
    virtual void  Free(void* memory)  = 0;
    virtual usize GetAllocationSize() = 0;
};

struct SlabHeader
{
    SlabAllocatorBase* slab;
};

template <usize Bytes>
class SlabAllocator : public SlabAllocatorBase
{
  public:
    SlabAllocator() = default;
    void Initialize()
    {
        firstFree = ToHigherHalfAddress<uintptr_t>(
            PhysicalMemoryManager::CallocatePages(1));
        auto available
            = 0x1000 - Math::AlignUp(sizeof(SlabHeader), allocationSize);
        auto slabPointer  = reinterpret_cast<SlabHeader*>(firstFree);
        slabPointer->slab = this;
        firstFree += Math::AlignUp(sizeof(SlabHeader), allocationSize);

        auto array = reinterpret_cast<usize*>(firstFree);
        auto max   = available / allocationSize - 1;
        auto fact  = allocationSize / 8;
        for (usize i = 0; i < max; i++)
            array[i * fact] = reinterpret_cast<usize>(&array[(i + 1) * fact]);
        array[max * fact] = 0;
    }

    void* Allocate() override
    {
        ScopedLock guard(m_Lock);
        if (!firstFree) Initialize();

        auto oldFree = reinterpret_cast<usize*>(firstFree);
        firstFree    = oldFree[0];
        std::memset(oldFree, 0, allocationSize);

        return oldFree;
    }
    void Free(void* memory) override
    {
        ScopedLock guard(m_Lock);
        if (!memory) return;

        auto newHead = static_cast<usize*>(memory);
        newHead[0]   = firstFree;
        firstFree    = reinterpret_cast<uintptr_t>(newHead);
    }

    virtual usize GetAllocationSize() override { return allocationSize; }

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
    uintptr_t firstFree      = 0;
    usize     allocationSize = Bytes;
    Spinlock  m_Lock;
};
