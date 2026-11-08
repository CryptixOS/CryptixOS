/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Debug/Kasan.hpp>

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

        // How many bytes an allocation really has: its slab's object size,
        // or the pages of a large (page aligned) one
        usize Capacity(Pointer memory)
        {
            if ((memory.Raw() & (PMM::PAGE_SIZE - 1)) == 0)
                return memory.Offset<Pointer>(-PMM::PAGE_SIZE)
                           .As<BigAllocMeta>()
                           ->PageCount
                     * PMM::PAGE_SIZE;

            return Pointer(memory.Raw() & ~(PMM::PAGE_SIZE - 1))
                .As<Prism::SlabFrame>()
                ->Allocator->AllocationSize();
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

    // With KASAN, the bytes past the requested size and freed objects are
    // poisoned (Kasan::OnAllocate()/OnFree()), so everything here only ever
    // touches the requested bytes: the pool's own Callocate() and
    // Reallocate() would fill or copy whole objects.
    Pointer Allocate(usize bytes)
    {
        // EarlyPanic(): formatting a Panic() message would allocate again
        if (!s_Initialized) [[unlikely]]
            EarlyPanic("KernelHeap: allocation before Initialize()");
        Pointer memory = s_SlabPool->Allocate(bytes);
        if (memory) Kasan::OnAllocate(memory, bytes, Capacity(memory));
        return memory;
    }
    Pointer Callocate(usize bytes)
    {
        Pointer memory = Allocate(bytes);
        if (memory) Memory::Fill(memory, 0, bytes);
        return memory;
    }
    Pointer Reallocate(Pointer memory, usize size)
    {
        // EarlyPanic(): formatting a Panic() message would allocate again
        if (!s_Initialized) [[unlikely]]
            EarlyPanic("KernelHeap: allocation before Initialize()");
        if (!memory) return Allocate(size);
        if (!size)
        {
            Free(memory);
            return nullptr;
        }

        usize capacity = Capacity(memory);
        if (size <= capacity)
        {
            Kasan::OnAllocate(memory, size, capacity);
            return memory;
        }

        Pointer newMemory = Allocate(size);
        if (!newMemory) return nullptr;

        usize oldSize = Kasan::AddressableSize(memory, capacity);
        Memory::Copy(newMemory, memory, Min(oldSize, size));
        Free(memory);
        return newMemory;
    }
    void Free(Pointer memory)
    {
        // operator delete(nullptr) must be a no-op; the pool would otherwise
        // treat a null pointer as a large allocation and read its metadata.
        if (!memory) return;

        Kasan::OnFree(memory, Capacity(memory));
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
