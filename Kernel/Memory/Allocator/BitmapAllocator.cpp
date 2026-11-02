/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Math.hpp>

module Cryptix:Impl.Memory.Allocator.BitmapAllocator;

import :Memory.Allocator.BitmapAllocator;

BitmapAllocator::BitmapAllocator() = default;

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
            case MemoryZoneType::eACPI_Reclaimable:
            case MemoryZoneType::eBootloaderReclaimable:
            case MemoryZoneType::eKernelAndModules:
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
    LogInfo("PMM: Allocated {}KiB bitmap for {} pages", bitmapEntryCount / 1024, bitmapEntryCount);

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
