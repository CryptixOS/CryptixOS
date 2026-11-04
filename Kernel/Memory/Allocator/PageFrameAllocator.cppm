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

export module Cryptix:Memory.Allocator.PageFrameAllocator;

export import :Library.Locking.Spinlock;
export import :Memory.PMM;

export class PageFrameAllocator
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
