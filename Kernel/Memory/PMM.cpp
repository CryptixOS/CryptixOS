/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Debug/Debug.hpp>
#include <Debug/Kasan.hpp>
#include <Library/Locking/Spinlock.hpp>

#include <Memory/PMM.hpp>

#include <Memory/Allocator/BitmapAllocator.hpp>
#include <Memory/Allocator/KernelHeap.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

namespace PMM
{
    namespace
    {
        bool            s_Initialized = false;

        MemoryMap       s_MemoryMap{};
        // A copy of the zones before EarlyAllocatePages() shrinks them
        constexpr usize MAX_INITIAL_ZONES = 256;
        MemoryZone      s_InitialZones[MAX_INITIAL_ZONES];
        usize           s_InitialZoneCount = 0;
        BitmapAllocator s_BitmapAllocator;

        Pointer         EarlyAllocatePages(usize count = 1)
        {
            auto& memoryMap  = s_MemoryMap;
            usize entryCount = memoryMap.EntryCount;

            for (usize i = 0; i < entryCount; i++)
            {
                auto& entry = memoryMap.Entries[i];
                if (entry.Type() != MemoryZoneType::eUsable
                    || entry.Length() < count * PAGE_SIZE)
                    continue;

                auto pages = entry.Allocate(count * PAGE_SIZE);
                return pages;
            }

            return nullptr;
        }
    } // namespace

    void EarlyInitialize(const MemoryMap& memoryMap)
    {
        s_MemoryMap        = memoryMap;
        s_InitialZoneCount = Min(memoryMap.EntryCount, MAX_INITIAL_ZONES);
        for (usize i = 0; i < s_InitialZoneCount; i++)
            s_InitialZones[i] = memoryMap.Entries[i];
    }
    bool Initialize(const MemoryMap& memoryMap)
    {
        EarlyLogTrace("PMM: Initializing...");

        s_MemoryMap     = memoryMap;
        auto entryCount = s_MemoryMap.EntryCount;
        if (entryCount == 0) return false;

#if CTOS_PMM_DUMP_MEMORY_MAP
        for (usize i = 0; i < entryCount; i++)
        {
            auto&   current     = s_MemoryMap.Entries[i];
            Pointer entryBase   = current.Base();
            usize   entryLength = current.Length();

            EarlyLogDebug(
                "PMM: MemoryMap[%zu] = { .Base: %#p, .Length: %#x, .Type: "
                "%s }",
                i, entryBase.Raw(), entryLength, ToString(current.Type()));
        }
#endif

        auto status = s_BitmapAllocator.Initialize(s_MemoryMap, PAGE_SIZE);
        if (!status)
        {
            LogError(
                "PMM: Failed to initialize bitmap allocator, the error code: "
                "{}",
                ToString(status.Error()));
            return false;
        }
        EarlyLogInfo("PMM: Initialized");

        EarlyLogInfo(
            "PMM: Memory Map entry count: %zu, Total Memory: %zuMiB, Free "
            "Memory%zuMiB ",
            entryCount, s_BitmapAllocator.TotalMemorySize() / 1024 / 1024,
            GetFreeMemory() / 1024 / 1024);

        return (s_Initialized = true);
    }
    bool             IsInitialized() { return s_Initialized; }

    Span<MemoryZone> MemoryZones()
    {
        return Span(s_MemoryMap.Entries, s_MemoryMap.EntryCount);
    }
    Span<const MemoryZone> InitialMemoryZones()
    {
        return Span<const MemoryZone>(s_InitialZones, s_InitialZoneCount);
    }

    void* AllocatePages(usize count)
    {
        Pointer pages = s_Initialized ? s_BitmapAllocator.AllocatePages(count)
                                      : EarlyAllocatePages(count);
        // Free pages are poisoned while KASAN runs
        if (pages) Kasan::Unpoison(pages.ToHigherHalf(), count * PAGE_SIZE);
        return pages;
    }
    void* CallocatePages(usize count)
    {
        Pointer pages = AllocatePages(count);
        if (!pages) return nullptr;

        Memory::Fill(pages.ToHigherHalf(), 0, PAGE_SIZE * count);
        return pages;
    }

    void FreePages(void* ptr, usize count)
    {
        // Early allocations come straight from the memory map and aren't
        // tracked by any bitmap yet, so there is nothing to give them back to.
        if (!s_Initialized) return;

        Kasan::Poison(Pointer(ptr).ToHigherHalf(), count * PAGE_SIZE,
                      Kasan::ShadowType::ePageFree);
        return s_BitmapAllocator.FreePages(ptr, count);
    }

    void ForEachFreeRange(void (*callback)(Pointer base, usize pageCount))
    {
        if (s_Initialized) s_BitmapAllocator.ForEachFreeRange(callback);
    }

    uintptr_t GetMemoryTop() { return s_BitmapAllocator.MemoryTop(); }
    u64       GetTotalMemory() { return s_BitmapAllocator.TotalMemorySize(); }
    u64       GetFreeMemory() { return GetTotalMemory() - GetUsedMemory(); }
    u64       GetUsedMemory() { return s_BitmapAllocator.UsedMemorySize(); }
} // namespace PMM
