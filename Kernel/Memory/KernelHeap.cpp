/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "KernelHeap.hpp"

#include "Memory/PMM.hpp"
#include "Memory/SlabAllocator.hpp"

namespace KernelHeap
{
    namespace
    {
        SlabAllocator<8>    allocator8    = {};
        SlabAllocator<16>   allocator16   = {};
        SlabAllocator<32>   allocator32   = {};
        SlabAllocator<64>   allocator64   = {};
        SlabAllocator<128>  allocator128  = {};
        SlabAllocator<256>  allocator256  = {};
        SlabAllocator<512>  allocator512  = {};
        SlabAllocator<1024> allocator1024 = {};

        struct BigAllocMeta
        {
            usize pages;
            usize size;
        };

        void* InternalAllocate(usize size)
        {
            if (size <= 8) return allocator8.Allocate<void*>();
            else if (size <= 16) return allocator16.Allocate<void*>();
            else if (size <= 32) return allocator32.Allocate<void*>();
            else if (size <= 64) return allocator64.Allocate<void*>();
            else if (size <= 128) return allocator128.Allocate<void*>();
            else if (size <= 256) return allocator256.Allocate<void*>();
            else if (size <= 512) return allocator512.Allocate<void*>();
            else if (size <= 1024) return allocator1024.Allocate<void*>();

            usize pages = Math::AlignUp(size, 4096) / 4096;
            auto  ptr
                = PhysicalMemoryManager::CallocatePages<uintptr_t>(pages + 1);
            if (ptr == 0) return nullptr;

            auto meta = reinterpret_cast<BigAllocMeta*>(
                ToHigherHalfAddress<void*>(ptr));
            meta->pages = pages;
            meta->size  = size;

            return reinterpret_cast<void*>(ToHigherHalfAddress<uintptr_t>(ptr)
                                           + PMM::PAGE_SIZE);
        }

        void InternalFree(void* ptr)
        {
            if ((reinterpret_cast<uintptr_t>(ptr) & 0xfff) == 0)
            {
                auto meta = reinterpret_cast<BigAllocMeta*>(
                    reinterpret_cast<uintptr_t>(ptr) - PMM::PAGE_SIZE);
                PhysicalMemoryManager::FreePages(
                    FromHigherHalfAddress<void*>((uintptr_t)meta),
                    meta->pages + 1);
                return;
            }
            reinterpret_cast<SlabHeader*>(reinterpret_cast<uintptr_t>(ptr)
                                          & ~0xfff)
                ->slab->Free(ptr);
        }
    } // namespace
      //
    void Initialize()
    {
        EarlyLogTrace("KernelHeap: Initializing...");

        allocator8.Initialize();
        allocator16.Initialize();
        allocator32.Initialize();
        allocator64.Initialize();
        allocator128.Initialize();
        allocator256.Initialize();
        allocator512.Initialize();
        allocator1024.Initialize();

        LogInfo("KernelHeap: Initialized");
    }

    void* Allocate(usize bytes) { return InternalAllocate(bytes); }
    void* Callocate(usize bytes)
    {
        void* ret = InternalAllocate(bytes);
        if (!ret) return nullptr;

        std::memset(ret, 0, bytes);
        return ret;
    }
    void* Reallocate(void* ptr, usize size)
    {
        if (!ptr) return Allocate(size);
        if ((reinterpret_cast<uintptr_t>(ptr) & 0xfff) == 0)
        {
            BigAllocMeta* metadata = reinterpret_cast<BigAllocMeta*>(
                reinterpret_cast<uintptr_t>(ptr) - 0x1000);
            usize oldSize = metadata->size;

            if (Math::DivRoundUp(oldSize, 0x1000)
                == Math::DivRoundUp(size, 0x1000))
            {
                metadata->size = size;
                return ptr;
            }

            if (size == 0)
            {
                Free(ptr);
                return nullptr;
            }

            if (size < oldSize) oldSize = size;

            void* ret = Allocate(size);
            if (!ret) return ptr;

            std::memcpy(ret, ptr, oldSize);
            Free(ptr);
            return ret;
        }

        SlabAllocatorBase* slab = reinterpret_cast<SlabHeader*>(
                                      reinterpret_cast<uintptr_t>(ptr) & ~0xFFF)
                                      ->slab;
        usize oldSize = slab->GetAllocationSize();

        if (size == 0)
        {
            Free(ptr);
            return nullptr;
        }
        if (size < oldSize) oldSize = size;

        void* ret = Allocate(size);
        if (!ret) return ptr;

        memcpy(ret, ptr, oldSize);
        Free(ptr);
        return ptr;
    }
    void Free(void* memory)
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
void operator delete(void* ptr) noexcept { KernelHeap::Free(ptr); }
void operator delete(void* ptr, std::align_val_t) noexcept
{
    KernelHeap::Free(ptr);
}
void operator delete(void* ptr, usize) noexcept { KernelHeap::Free(ptr); }
void operator delete[](void* ptr) noexcept { KernelHeap::Free(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept
{
    KernelHeap::Free(ptr);
}
void operator delete[](void* ptr, usize) noexcept { KernelHeap::Free(ptr); }
