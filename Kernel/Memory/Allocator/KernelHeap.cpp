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

namespace VMM
{
    PageMap* GetKernelPageMap();
}
namespace KernelHeap
{
    struct PageAllocPolicy
    {
        static Pointer CallocatePages(usize pageCount = 1)
        {
            return Pointer(PMM::CallocatePages(pageCount)).ToHigherHalf();
        }
        static void FreePages(Pointer memory, usize pageCount = 1)
        {
            PMM::FreePages(memory.FromHigherHalf(), pageCount);
        }
    };
    class SpinLockPolicy
    {
      public:
        void                     Init() {}
        [[nodiscard]] ScopedLock Lock() { return ScopedLock(m_Lock); }

      private:
        Spinlock m_Lock;
    };

    namespace
    {
        constexpr usize BUCKET_COUNT         = 8;

        bool            s_Initialized        = false;
        Pointer         s_EarlyHeapBase      = nullptr;
        usize           s_EarlyHeapSize      = 0;
        usize           s_EarlyHeapAllocated = 0;
        Pointer         s_EarlyHeapCurrent   = nullptr;

        alignas(SlabPool<8, PageAllocPolicy, SpinLockPolicy>) static u8
            s_SlabPoolStorage[sizeof(
                SlabPool<8, PageAllocPolicy, SpinLockPolicy>)];
        static SlabPool<8, PageAllocPolicy, SpinLockPolicy>* s_SlabPool
            = nullptr;

        Pointer EarlyHeapAllocate(usize size)
        {
            size = Math::AlignUp(size, sizeof(void*));
            Assert(s_EarlyHeapSize - s_EarlyHeapAllocated > size);

            auto memory = s_EarlyHeapCurrent;
            s_EarlyHeapCurrent += size;
            s_EarlyHeapAllocated += size;

            return memory;
        }
    } // namespace

    void EarlyInitialize(Pointer base, usize length)
    {
        s_EarlyHeapBase    = base;
        s_EarlyHeapSize    = length;
        s_EarlyHeapCurrent = s_EarlyHeapBase;
    }
    void MapEarlyHeap()
    {
        auto  baseVirt  = s_EarlyHeapBase;
        usize pageCount = Math::DivRoundUp(s_EarlyHeapSize, PMM::PAGE_SIZE);

        auto  pageMap   = VMM::GetKernelPageMap();
        for (usize i = 0; i < pageCount; i++)
        {
            auto offset = i * PMM::PAGE_SIZE;
            auto virt   = baseVirt.Offset<Pointer>(offset);
            auto phys   = virt.FromHigherHalf();

            if (pageMap->Virt2Phys(virt)) continue;
            Assert(pageMap->Map(virt, phys));
        }
    }

    void Initialize()
    {
        EarlyLogTrace("KernelHeap: Initializing...");
        s_SlabPool = new (&s_SlabPoolStorage)
            SlabPool<8, PageAllocPolicy, SpinLockPolicy>();
        s_SlabPool->Initialize();

        s_Initialized = true;
        LogInfo("KernelHeap: Initialized `{}` slab buckets", BUCKET_COUNT);
    }

    Pointer Allocate(usize bytes)
    {
        if (!s_Initialized) return EarlyHeapAllocate(bytes);
        return s_SlabPool->Allocate(bytes);
    }
    Pointer Callocate(usize bytes)
    {
        return s_Initialized ? s_SlabPool->Callocate(bytes)
                             : EarlyHeapAllocate(bytes);
    }
    Pointer Reallocate(Pointer memory, usize size)
    {
        return s_SlabPool->Reallocate(memory, size);
    }
    void Free(Pointer memory) { return s_SlabPool->Free(memory); }
} // namespace KernelHeap

void* operator new(usize size) { return KernelHeap::Callocate(size); }
void* operator new(usize size, std::align_val_t)
{
    return KernelHeap::Callocate(size);
}
void* operator new[](usize size) { return KernelHeap::Callocate(size); }
void* operator new[](usize size, std::align_val_t)
{
    return KernelHeap::Callocate(size);
}
void operator delete(void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete(void* memory, std::align_val_t) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete(void* memory, usize) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete[](void* memory, usize) noexcept
{
    KernelHeap::Free(memory);
}
