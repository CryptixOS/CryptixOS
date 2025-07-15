/*
 * Created by v1tr10l7 on 15.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/Allocator/SlabAllocator.hpp>
#include <Prism/Memory/Allocator.hpp>

struct BigAllocMeta
{
    usize PageCount;
    usize Size;
};

template <usize BucketCount, typename PageAllocPolicy, typename LockPolicy>
class SlabPool : public AllocatorBase
{
  public:
    virtual ErrorOr<void> Initialize() override
    {
        for (usize i = 0; i < BucketCount; i++)
            Assert(m_Buckets[i].Initialize(8 << i));

        return {};
    }
    virtual void    Shutdown() override {}

    virtual Pointer Allocate(usize bytes, usize alignment = 0) override
    {
        if (bytes > MAX_BUCKET_SIZE)
        {
            auto memory = LargeAllocate(bytes);
            if (memory) return memory;
        }

        for (usize i = 0; i < BucketCount; i++)
        {
            if (bytes > (8zu << i)) continue;

            auto memory = m_Buckets[i].Allocate();
            if (!memory) continue;

            m_TotalAllocated += m_Buckets[i].AllocationSize();
            return memory;
        }
        return nullptr;
    }
    virtual void Free(Pointer memory) override
    {
        if ((memory.Raw() & 0xfff) == 0) return LargeFree(memory);

        auto allocator = Pointer(memory.Raw() & ~0xfff).As<SlabHeader>();
        allocator->Slab->Free(memory);
        m_TotalFreed += allocator->Slab->AllocationSize();
    }

    Pointer LargeAllocate(usize bytes)
    {
        usize   pageCount = Math::DivRoundUp(bytes, PMM::PAGE_SIZE);
        Pointer memory    = PageAllocPolicy::CallocatePages(pageCount + 1);
        if (!memory) return nullptr;

        auto meta       = memory.As<BigAllocMeta>();
        meta->PageCount = pageCount;
        meta->Size      = bytes;

        m_TotalAllocated += meta->Size;
        return memory.Offset(PMM::PAGE_SIZE);
    }
    void LargeFree(Pointer memory)
    {
        if (!(memory.Raw() & 0xfff)) return;

        auto meta = memory.Offset<Pointer>(-PMM::PAGE_SIZE).As<BigAllocMeta>();

        m_TotalFreed += meta->Size;
        PageAllocPolicy::FreePages(meta, meta->PageCount);
    }

    virtual usize TotalAllocated() const override { return m_TotalAllocated; }
    virtual usize TotalFreed() const override { return m_TotalFreed; }
    virtual usize Used() const override
    {
        return TotalAllocated() - TotalFreed();
    }

  private:
    constexpr static usize MAX_BUCKET_SIZE = 4 << BucketCount;
    Array<SlabAllocator<PageAllocPolicy, LockPolicy>, BucketCount> m_Buckets;
    LockPolicy                                                     m_Lock;

    usize m_TotalAllocated = 0;
    usize m_TotalFreed     = 0;
};
