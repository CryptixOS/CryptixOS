/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/Allocators/BitmapAllocator.hpp>

ErrorOr<void> BitmapAllocator::Initialize(MemoryMap& memoryMap, usize pageSize)
{
    if (!Math::IsPowerOfTwo(pageSize)) return Error(EINVAL);
    m_PageSize = pageSize;

    for (usize i = 0; i < memoryMap.EntryCount; i++)
    {
        auto&   entry = memoryMap.Entries[i];
        Pointer top   = entry.Base().Offset(entry.Size());
        m_MemoryTop   = std::max(m_MemoryTop.Raw(), top.Raw());

        switch (entry.Type())
        {
            case MemoryType::eUsable:
                m_UsableMemorySize += entry.Size();
                m_UsableMemoryTop = std::max(m_UsableMemoryTop, top);

                break;
            case MemoryType::eACPI_Reclaimable:
            case MemoryType::eBootloaderReclaimable:
            case MemoryType::eKernelAndModules:
                m_UsedMemory += entry.Size();
                break;
            default: continue;
        }

        m_TotalMemory += entry.Size();
    }

    if (!m_MemoryTop) return Error(ENOMEM);

    usize bitmapEntryCount = m_UsableMemoryTop.Raw() / pageSize;
    m_PageBitmap.Allocate(bitmapEntryCount);
    m_PageBitmap.SetAll(0xff);

    Assert(m_PageBitmap.GetSize() != 0);
    for (usize i = 0; i < memoryMap.EntryCount; i++)
    {
        auto& current = memoryMap.Entries[i];
        if (current.Type() != MemoryType::eUsable) continue;

        for (usize page = current.Base().Raw() == 0 ? 4096 : 0;
             page < current.Size(); page += pageSize)
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
    ScopedLock guard(m_Lock);

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
    ScopedLock guard(m_Lock);
    if (pageCount == 0 || !page) return;

    usize pageIndex = page.Raw() / m_PageSize;
    for (usize i = pageIndex; i < pageIndex + pageCount; i++)
        m_PageBitmap.SetIndex(i, false);

    m_UsedMemory -= pageCount * m_PageSize;
}

Pointer BitmapAllocator::FindFreeRegions(usize& start, usize count, usize limit)
{
    usize contiguousPages = 0;
    while (start < limit)
    {
        if (!m_PageBitmap.GetIndex(start++))
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
