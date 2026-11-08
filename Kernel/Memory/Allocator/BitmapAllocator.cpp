/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/Allocator/BitmapAllocator.hpp>
#include <Prism/Utility/Math.hpp>
#include <Library/Stacktrace.hpp> // DBGMAP

ErrorOr<void> BitmapAllocator::Initialize(MemoryMap& memoryMap, usize pageSize)
{
    if (!Math::IsPowerOfTwo(pageSize)) return Error(EINVAL);
    m_PageSize = pageSize;

    for (usize i = 0; i < memoryMap.EntryCount; i++)
    {
        auto&   entry = memoryMap.Entries[i];
        Pointer top   = entry.Base().Offset(entry.Length());
        m_MemoryTop   = Math::Max(m_MemoryTop.Raw(), top.Raw());

        switch (entry.Type())
        {
            case MemoryZoneType::eUsable:
                m_UsableMemorySize += entry.Length();
                m_UsableMemoryTop = Math::Max(m_UsableMemoryTop, top);

                break;
            // In use for now, but given to the PMM later: the bootloader's
            // memory and the kernel's init section once the kernel is up, so
            // the bitmap covers them too.
            case MemoryZoneType::eBootloaderReclaimable:
            case MemoryZoneType::eKernelAndModules:
                m_UsableMemoryTop = Math::Max(m_UsableMemoryTop, top);
                m_UsedMemory += entry.Length();
                break;
            case MemoryZoneType::eACPI_Reclaimable:
                m_UsedMemory += entry.Length();
                break;
            default: continue;
        }

        m_TotalMemory += entry.Length();
    }

    if (!m_MemoryTop) return Error(ENOMEM);

    usize bitmapEntryCount = m_UsableMemoryTop.Raw() / pageSize;
    m_PageBitmap.Allocate(bitmapEntryCount);
    m_PageBitmap.SetAll(0xff);
    LogInfo("PMM: Allocated {}KiB bitmap for {} pages",
            Math::DivRoundUp(bitmapEntryCount, 8) / 1024, bitmapEntryCount);

    Assert(m_PageBitmap.BitCount() != 0);
    for (usize i = 0; i < memoryMap.EntryCount; i++)
    {
        auto& current = memoryMap.Entries[i];
        if (current.Type() != MemoryZoneType::eUsable) continue;

        for (usize page = current.Base().Raw() == 0 ? 4096 : 0;
             page < current.Length(); page += pageSize)
            m_PageBitmap.SetIndex((current.Base().Raw() + page) / pageSize,
                                  false);
    }

    return {};
}
void  BitmapAllocator::Shutdown() {}

usize BitmapAllocator::MemoryTop() const { return m_UsableMemoryTop; }
usize BitmapAllocator::TotalMemorySize() const { return m_TotalMemory; }
usize BitmapAllocator::FreeMemorySize() const
{
    return TotalMemorySize() - UsedMemorySize();
}
usize   BitmapAllocator::UsedMemorySize() const { return m_UsedMemory; }
usize   BitmapAllocator::PageSize() const { return m_PageSize; }

Pointer BitmapAllocator::AllocatePages(usize pageCount)
{
    // Disable interrupts while held: interrupt handlers (and the page fault
    // handler) allocate pages, which would deadlock on this CPU otherwise.
    ScopedLock guard(m_Lock, true);

    if (pageCount == 0) return nullptr;
    static usize lastIndex = 0;

    usize        i         = lastIndex;
    auto         pages     = FindFreeRegions(lastIndex, pageCount,
                                             m_UsableMemoryTop.Raw() / m_PageSize);

    if (!pages)
    {
        lastIndex = 0;
        pages     = FindFreeRegions(lastIndex, pageCount, i);

        if (!pages) return nullptr;
    }

    m_UsedMemory += pageCount * m_PageSize;
    return pages;
}
void BitmapAllocator::FreePages(Pointer page, usize pageCount)
{
    // Disable interrupts while held: interrupt handlers (and the page fault
    // handler) allocate pages, which would deadlock on this CPU otherwise.
    ScopedLock guard(m_Lock, true);
    if (pageCount == 0 || !page) return;

    usize pageIndex = page.Raw() / m_PageSize;
    usize pageLimit = m_PageBitmap.BitCount();
    if (pageIndex >= pageLimit || pageCount > pageLimit - pageIndex)
    {
        LogError("PMM: FreePages({:#x}, {}) is outside of managed memory",
                 page.Raw(), pageCount);
        return;
    }

    usize freed = 0;
    for (usize i = pageIndex; i < pageIndex + pageCount; i++)
    {
        if (!m_PageBitmap.At(i))
        {
            LogError("PMM: Double free of page {:#x}", i * m_PageSize);
            if (i == pageIndex) Stacktrace::Print(); // DBGMAP
            continue;
        }
        m_PageBitmap.SetIndex(i, false);
        ++freed;
    }

    // Pages handed out before the bitmap existed (early allocations) were
    // never added to m_UsedMemory, so don't let the counter underflow.
    usize freedBytes = freed * m_PageSize;
    m_UsedMemory     = freedBytes > m_UsedMemory ? 0 : m_UsedMemory - freedBytes;
}

void BitmapAllocator::ForEachFreeRange(
    void (*callback)(Pointer base, usize pageCount))
{
    ScopedLock guard(m_Lock, true);

    usize      pageCount = m_PageBitmap.BitCount();
    for (usize page = 0; page < pageCount;)
    {
        if (m_PageBitmap.At(page))
        {
            ++page;
            continue;
        }

        usize first = page;
        while (page < pageCount && !m_PageBitmap.At(page)) ++page;
        callback(first * m_PageSize, page - first);
    }
}

Pointer BitmapAllocator::FindFreeRegions(usize& start, usize count, usize limit)
{
    usize contiguousPages = 0;
    while (start < limit)
    {
        if (!m_PageBitmap.At(start++))
        {
            if (++contiguousPages == count)
            {
                usize page = start - count;
                for (usize i = page; i < start; i++)
                    m_PageBitmap.SetIndex(i, true);

                return page * m_PageSize;
            }
            continue;
        }

        contiguousPages = 0;
    }

    return nullptr;
}
