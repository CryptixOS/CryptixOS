/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Boot/BootInfo.hpp>
#include <Utility/Bitmap.hpp>
#include <Utility/Math.hpp>

#include <Memory/KernelHeap.hpp>
#include <Memory/PMM.hpp>

#include <Utility/Spinlock.hpp>

namespace PhysicalMemoryManager
{
    namespace
    {
        bool      s_Initialized = false;
        Bitmap    s_Bitmap{};

        uintptr_t s_MemoryTop        = 0;
        uintptr_t s_UsableMemoryTop  = 0;

        usize     s_UsableMemorySize = 0;
        usize     s_TotalMemory      = 0;
        usize     s_UsedMemory       = 0;
        Spinlock  s_Lock;

        void*     FindFreeRegion(usize& start, usize count, usize limit)
        {
            usize contiguousPages = 0;
            while (start < limit)
            {
                if (!s_Bitmap.GetIndex(start++))
                {
                    if (++contiguousPages == count)
                    {
                        usize page = start - count;
                        for (usize i = page; i < start; i++)
                            s_Bitmap.SetIndex(i, true);

                        return reinterpret_cast<void*>(page * PAGE_SIZE);
                    }
                }
                else contiguousPages = 0;
            }

            return nullptr;
        }
    } // namespace

    bool Initialize()
    {
        usize     entryCount = 0;
        MemoryMap memoryMap  = BootInfo::GetMemoryMap(entryCount);
        if (entryCount == 0) return false;

        EarlyLogTrace("PMM: Initializing...");
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            uintptr_t       top = currentEntry->base + currentEntry->length;
            s_MemoryTop         = std::max(s_MemoryTop, top);

            switch (currentEntry->type)
            {
                case MEMORY_MAP_USABLE:
                    s_UsableMemorySize += currentEntry->length;
                    s_UsableMemoryTop = std::max(s_UsableMemoryTop, top);
                    break;
                case MEMORY_MAP_ACPI_RECLAIMABLE:
                case MEMORY_MAP_BOOTLOADER_RECLAIMABLE:
                case MEMORY_MAP_KERNEL_AND_MODULES:
                    s_UsedMemory += currentEntry->length;
                    break;
                default: continue;
            }

            s_TotalMemory += currentEntry->length;
        }

        if (s_MemoryTop == 0) return false;

        usize s_BitmapEntryCount = s_UsableMemoryTop / PAGE_SIZE;
        usize s_BitmapSize = Math::AlignUp(s_BitmapEntryCount / 8, PAGE_SIZE);
        s_BitmapEntryCount = s_BitmapSize * 8;
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            if (currentEntry->type != MEMORY_MAP_USABLE
                || currentEntry->length < s_BitmapSize)
                continue;

            s_Bitmap.data = reinterpret_cast<u8*>(currentEntry->base
                                                  + BootInfo::GetHHDMOffset());
            s_Bitmap.size = s_BitmapEntryCount;
            s_Bitmap.SetAll(0xff);
            currentEntry->base += s_BitmapSize;
            currentEntry->length -= s_BitmapSize;

            s_UsedMemory += s_BitmapSize;
            break;
        }

        Assert(s_Bitmap.size != 0);
        [[maybe_unused]] auto entryTypeToString = [](u64 type)
        {
            switch (type)
            {
                case LIMINE_MEMMAP_USABLE: return "Usable";
                case LIMINE_MEMMAP_RESERVED: return "Reserved";
                case LIMINE_MEMMAP_ACPI_RECLAIMABLE: return "ACPI Reclaimable";
                case LIMINE_MEMMAP_ACPI_NVS: return "ACPI NVS";
                case LIMINE_MEMMAP_BAD_MEMORY: return "Bad Memory";
                case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                    return "Bootloader Reclaimable";
                case MEMORY_MAP_KERNEL_AND_MODULES: return "Kernel and Modules";
                case LIMINE_MEMMAP_FRAMEBUFFER: return "Framebuffer";

                default: break;
            }

            return "Undefined";
        };

        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* currentEntry = memoryMap[i];
            /*EarlyLogTrace("MemoryMap[%zu]: base: %#zx, size: %zuKiB, type:
               %s", i, currentEntry->base, currentEntry->length / 1024,
                          entryTypeToString(currentEntry->type));*/

            if (currentEntry->type != MEMORY_MAP_USABLE) continue;

            for (uintptr_t page = currentEntry->base == 0 ? 4096 : 0;
                 page < currentEntry->length; page += PAGE_SIZE)
                s_Bitmap.SetIndex((currentEntry->base + page) / PAGE_SIZE,
                                  false);
        }

        // s_MemoryTop = Math::AlignUp(s_MemoryTop, 0x40000000);
        EarlyLogInfo("PMM: Initialized");

        EarlyLogInfo(
            "PMM: Memory Map entry count: %zu, Total Memory: %zuMiB, Free "
            "Memory%zuMiB ",
            entryCount, s_TotalMemory / 1024 / 1024,
            GetFreeMemory() / 1024 / 1024);

        KernelHeap::Initialize();

        return (s_Initialized = true);
    }
    bool  IsInitialized() { return s_Initialized; }

    void* AllocatePages(usize count)
    {
        ScopedLock guard(s_Lock);
        if (count == 0) return nullptr;
        static usize lastIndex = 0;

        usize        i         = lastIndex;
        void*        ret
            = FindFreeRegion(lastIndex, count, s_UsableMemoryTop / PAGE_SIZE);
        if (!ret)
        {
            lastIndex = 0;
            ret       = FindFreeRegion(lastIndex, count, i);
            if (!ret) EarlyPanic("Out of memory!");
        }

        s_UsedMemory += count * PAGE_SIZE;
        return ret;
    }
    void* CallocatePages(usize count)
    {
        void* ret = AllocatePages(count);
        if (!ret) return nullptr;

        uintptr_t higherHalfAddress
            = reinterpret_cast<uintptr_t>(ret) + BootInfo::GetHHDMOffset();
        memset(reinterpret_cast<void*>(higherHalfAddress), 0,
               PAGE_SIZE * count);

        return ret;
    }

    void FreePages(void* ptr, usize count)
    {
        ScopedLock guard(s_Lock);
        if (count == 0 || !ptr) return;
        uintptr_t page = reinterpret_cast<uintptr_t>(ptr) / PAGE_SIZE;

        for (uintptr_t i = page; i < page + count; i++)
            s_Bitmap.SetIndex(i, false);
        s_UsedMemory -= count * PAGE_SIZE;
    }

    uintptr_t GetMemoryTop() { return s_UsableMemoryTop; }
    uint64_t  GetTotalMemory() { return s_TotalMemory; }
    uint64_t  GetFreeMemory() { return s_TotalMemory - s_UsedMemory; }
    uint64_t  GetUsedMemory() { return s_UsedMemory; }
} // namespace PhysicalMemoryManager
