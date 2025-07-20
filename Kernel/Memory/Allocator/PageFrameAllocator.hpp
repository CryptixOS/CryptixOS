/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>
#include <Memory/PMM.hpp>

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Memory.hpp>

class PageFrameAllocator
{
  public:
    PageFrameAllocator()                     = default;
    virtual ~PageFrameAllocator()            = default;

    constexpr static usize DEFAULT_PAGE_SIZE = 0x1000;

    virtual ErrorOr<void>  Initialize(MemoryMap& memoryMap,
                                      usize      pageSize = DEFAULT_PAGE_SIZE)
        = 0;
    virtual void    Shutdown()              = 0;

    virtual usize   MemoryTop() const       = 0;
    virtual usize   TotalMemorySize() const = 0;
    virtual usize   FreeMemorySize() const  = 0;
    virtual usize   UsedMemorySize() const  = 0;
    virtual usize   PageSize() const        = 0;

    virtual Pointer AllocatePage() { return AllocatePages(1); }
    virtual Pointer AllocatePages(usize pageCount) = 0;

    virtual Pointer CallocatePage() { return CallocatePages(1); }
    virtual Pointer CallocatePages(usize pageCount)
    {
        auto pages = AllocatePages(pageCount);
        if (!pages) return nullptr;

        Memory::Fill(pages.ToHigherHalf(), 0, pageCount * PageSize());
        return pages;
    }

    virtual void FreePage(Pointer page) { FreePages(page, 1); }
    virtual void FreePages(Pointer page, usize pageCount) = 0;

  protected:
    Spinlock m_Lock;
};
