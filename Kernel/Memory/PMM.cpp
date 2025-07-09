/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Boot/BootInfo.hpp>
#include <Library/Locking/Spinlock.hpp>

#include <Memory/Allocators/BitmapAllocator.hpp>
#include <Memory/KernelHeap.hpp>
#include <Memory/PMM.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

namespace PhysicalMemoryManager
{
    namespace
    {
        bool            s_Initialized = false;
        BitmapAllocator s_BitmapAllocator;

        Pointer         EarlyAllocatePages(usize count = 1)
        {
            auto& memoryMap  = BootInfo::MemoryMap();
            usize entryCount = memoryMap.EntryCount;

            for (usize i = 0; i < entryCount; i++)
            {
                auto& entry = memoryMap.Entries[i];
                if (entry.Type() != MemoryType::eUsable
                    || entry.Size() < count * PAGE_SIZE)
                    continue;

                auto pages = entry.m_Base;
                entry.m_Base += count * PAGE_SIZE;
                entry.m_Size -= count * PAGE_SIZE;

                return pages;
            }

            return nullptr;
        }
    } // namespace

    bool Initialize()
    {
        EarlyLogTrace("PMM: Initializing...");

        auto memoryMap  = BootInfo::MemoryMap();
        auto entryCount = memoryMap.EntryCount;
        if (entryCount == 0) return false;

        for (usize i = 0; i < entryCount; i++)
        {
            auto&   current     = memoryMap.Entries[i];
            Pointer entryBase   = current.Base();
            usize   entryLength = current.Size();

            EarlyLogDebug(
                "PMM: MemoryMap[%zu] = { .Base: %#p, .Length: %#x, .Type: "
                "%s }",
                i, entryBase.Raw(), entryLength, ToString(current.Type()));
        }

        auto status = s_BitmapAllocator.Initialize(memoryMap, PAGE_SIZE);
        if (!status)
        {
            LogError(
                "PMM: Failed to initialize bitmap allocator, the error code: "
                "{}",
                ToString(status.error()));
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
    bool  IsInitialized() { return s_Initialized; }

    void* AllocatePages(usize count)
    {
        if (!s_Initialized) return EarlyAllocatePages(count);
        return s_BitmapAllocator.AllocatePages(count);
    }
    void* CallocatePages(usize count)
    {
        Pointer pages = AllocatePages(count);
        if (!pages) return nullptr;

        memset(pages.ToHigherHalf(), 0, PAGE_SIZE * count);
        return pages;
    }

    void FreePages(void* ptr, usize count)
    {
        return s_BitmapAllocator.FreePages(ptr, count);
    }

    uintptr_t GetMemoryTop() { return s_BitmapAllocator.MemoryTop(); }
    u64       GetTotalMemory() { return s_BitmapAllocator.TotalMemorySize(); }
    u64       GetFreeMemory() { return GetTotalMemory() - GetUsedMemory(); }
    u64       GetUsedMemory() { return s_BitmapAllocator.UsedMemorySize(); }
} // namespace PhysicalMemoryManager
