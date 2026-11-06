/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Library/Locking/Spinlock.hpp>
#include <Memory/PMM.hpp>
#include <Memory/PageMap.hpp>

#include <Memory/Allocator/KernelHeap.hpp>
#include <Prism/Memory/SlabPool.hpp>

namespace KernelHeap
{
    struct PageAllocPolicy
    {
        static Pointer CallocatePages(usize pageCount = 1)
        {
            Pointer phys = PMM::CallocatePages(pageCount);
            return phys ? phys.ToHigherHalf() : Pointer(nullptr);
        }
        static void FreePages(Pointer memory, usize pageCount = 1)
        {
            PMM::FreePages(memory.FromHigherHalf<void*>(), pageCount);
        }
    };
    namespace
    {
        constexpr usize BUCKET_COUNT  = 8;

        bool            s_Initialized = false;

        using Pool = SlabPool<BUCKET_COUNT, PageAllocPolicy, SpinLockPolicy>;
        alignas(Pool) static u8 s_SlabPoolStorage[sizeof(Pool)];
        static Pool*            s_SlabPool = nullptr;

        // Slab objects are naturally aligned to their (power of two) size
        // class, and large allocations are page aligned, so rounding the
        // request up to the alignment satisfies any alignment <= PAGE_SIZE.
        inline usize            AlignedSize(usize size, usize alignment)
        {
            AssertMsg(alignment <= PMM::PAGE_SIZE,
                      "KernelHeap: alignment above PAGE_SIZE is unsupported");
            return Max(size, alignment);
        }
    } // namespace

    void Initialize()
    {
        EarlyLogTrace("KernelHeap: Initializing...");
        s_SlabPool = new (&s_SlabPoolStorage) Pool();
        s_SlabPool->Initialize();

        s_Initialized = true;
        LogInfo("KernelHeap: Initialized `{}` slab buckets", BUCKET_COUNT);
    }

    Pointer Allocate(usize bytes)
    {
        AssertMsg(s_Initialized, "KernelHeap: allocation before Initialize()");
        return s_SlabPool->Allocate(bytes);
    }
    Pointer Callocate(usize bytes)
    {
        AssertMsg(s_Initialized, "KernelHeap: allocation before Initialize()");
        return s_SlabPool->Callocate(bytes);
    }
    Pointer Reallocate(Pointer memory, usize size)
    {
        AssertMsg(s_Initialized, "KernelHeap: allocation before Initialize()");
        return s_SlabPool->Reallocate(memory, size);
    }
    void Free(Pointer memory)
    {
        // operator delete(nullptr) must be a no-op; the pool would otherwise
        // treat a null pointer as a large allocation and read its metadata.
        if (!memory) return;
        s_SlabPool->Free(memory);
    }
} // namespace KernelHeap

void* operator new(usize size) { return KernelHeap::Callocate(size); }
void* operator new(usize size, AlignmentType alignment)
{
    return KernelHeap::Callocate(
        KernelHeap::AlignedSize(size, static_cast<usize>(alignment)));
}
void* operator new[](usize size) { return KernelHeap::Callocate(size); }
void* operator new[](usize size, AlignmentType alignment)
{
    return KernelHeap::Callocate(
        KernelHeap::AlignedSize(size, static_cast<usize>(alignment)));
}
void operator delete(void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete(void* memory, AlignmentType) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete(void* memory, usize) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory, AlignmentType) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete[](void* memory, usize) noexcept
{
    KernelHeap::Free(memory);
}
