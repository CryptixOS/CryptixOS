/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>
#include <Memory/PMM.hpp>

#include <Memory/Allocator/KernelHeap.hpp>
#include <Memory/Allocator/SlabAllocator.hpp>

namespace KernelHeap
{
    namespace
    {
        constexpr usize MAX_BUCKET_SIZE = 1024;
        constexpr usize BUCKET_COUNT    = Math::Log2CEval(MAX_BUCKET_SIZE) - 2;

        bool            s_Initialized   = false;
        Pointer         s_EarlyHeapBase = nullptr;
        usize           s_EarlyHeapSize = 0;
        usize           s_EarlyHeapAllocated = 0;
        Pointer         s_EarlyHeapCurrent   = nullptr;

        alignas(Array<SlabAllocator, BUCKET_COUNT>) static u8
            s_BucketStorage[sizeof(Array<SlabAllocator, BUCKET_COUNT>)];
        static Array<SlabAllocator, BUCKET_COUNT>* s_Buckets = nullptr;

        struct BigAllocMeta
        {
            usize PageCount;
            usize Size;
        };

        Pointer EarlyHeapAllocate(usize size)
        {
            size = Math::AlignUp(size, sizeof(void*));
            Assert(s_EarlyHeapSize - s_EarlyHeapAllocated > size);

            auto memory = s_EarlyHeapCurrent;
            s_EarlyHeapCurrent += size;
            s_EarlyHeapAllocated += size;

            return memory;
        }

        Pointer LargeAllocate(usize size)
        {
            usize   pageCount = Math::DivRoundUp(size, PMM::PAGE_SIZE);
            Pointer memory    = PMM::CallocatePages(pageCount + 1);
            if (!memory) return nullptr;

            auto meta   = memory.ToHigherHalf<BigAllocMeta*>();
            meta->PageCount = pageCount;
            meta->Size  = size;

            return memory.ToHigherHalf().Offset(PMM::PAGE_SIZE);
        }
        void LargeFree(Pointer memory)
        {
            if (!(memory.Raw() & 0xfff))
                return;
        
            auto meta = memory.Offset<Pointer>(-PMM::PAGE_SIZE).As<BigAllocMeta>();
            PMM::FreePages(Pointer(meta).FromHigherHalf(), meta->PageCount + 1);
        }

        Pointer InternalAllocate(usize size)
        {
            if (!s_Initialized) return EarlyHeapAllocate(size);
            if (size > MAX_BUCKET_SIZE)
            {
                auto memory = LargeAllocate(size);
                if (memory) return memory;
            }

            for (usize i = 0; i < BUCKET_COUNT; i++)
                if (size <= (8zu << i)) return (*s_Buckets)[i].Allocate();

            return nullptr;
        }

        void InternalFree(Pointer memory)
        {
            if ((memory.Raw() & 0xfff) == 0)
            {
                return LargeFree(memory);
            }
            auto allocator = Pointer(memory.Raw() & ~0xfff).As<SlabHeader>();
            allocator->Slab->Free(memory);
        }
    } // namespace

    void EarlyInitialize(Pointer base, usize length)
    {
        s_EarlyHeapBase    = base;
        s_EarlyHeapSize    = length;
        s_EarlyHeapCurrent = s_EarlyHeapBase;
    }
    void Initialize()
    {
        EarlyLogTrace("KernelHeap: Initializing...");

        s_Buckets = new (&s_BucketStorage) Array<SlabAllocator, BUCKET_COUNT>();
        for (usize index = 0; index < BUCKET_COUNT; index++)
            Assert((*s_Buckets)[index].Initialize(8 << index));

        s_Initialized = true;
        LogInfo("KernelHeap: Initialized `{}` slab buckets", BUCKET_COUNT);
    }

    Pointer Allocate(usize bytes) { return InternalAllocate(bytes); }
    Pointer Callocate(usize bytes)
    {
        auto memory = InternalAllocate(bytes);
        if (!memory) return nullptr;

        Memory::Fill(memory, 0, bytes);
        return memory;
    }
    Pointer Reallocate(Pointer memory, usize size)
    {
        if (!memory) return Allocate(size);
        if ((memory.Raw() & 0xfff) == 0)
        {
            BigAllocMeta* metadata
                = memory.Offset<BigAllocMeta*>(-PMM::PAGE_SIZE);
            usize oldSize = metadata->Size;

            if (Math::DivRoundUp(oldSize, PMM::PAGE_SIZE)
                == Math::DivRoundUp(size, PMM::PAGE_SIZE))
            {
                metadata->Size = size;
                return memory;
            }

            if (size == 0)
            {
                Free(memory);
                return nullptr;
            }

            if (size < oldSize) oldSize = size;

            auto newMemory = Allocate(size);
            if (!newMemory) return memory;

            Memory::Copy(newMemory, memory, oldSize);
            Free(memory);
            return newMemory;
        }

        AllocatorBase* slab
            = Pointer(memory.Raw() & ~0xfff).As<SlabHeader>()->Slab;
        usize oldSize = slab->AllocationSize();

        if (size == 0)
        {
            Free(memory);
            return nullptr;
        }
        if (size < oldSize) oldSize = size;

        auto newMemory = Allocate(size);
        if (!newMemory) return memory;

        Memory::Copy(newMemory, memory, oldSize);
        Free(memory);
        return newMemory;
    }
    void Free(Pointer memory)
    {
        if (!memory) return;
        InternalFree(memory);
    }
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
