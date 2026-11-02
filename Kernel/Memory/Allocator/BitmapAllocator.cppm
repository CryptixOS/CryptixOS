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

export module Cryptix:Memory.Allocator.BitmapAllocator;

export import :Memory.Allocator.PageFrameAllocator;

export class BitmapAllocator : public PageFrameAllocator
{
  public:
    BitmapAllocator();

    virtual ErrorOr<void> Initialize(MemoryMap& memoryMap,
                                     usize      pageSize) override;
    virtual void          Shutdown() override;

    virtual usize         MemoryTop() const override;
    virtual usize         TotalMemorySize() const override;
    virtual usize         FreeMemorySize() const override;
    virtual usize         UsedMemorySize() const override;
    virtual usize         PageSize() const override;

    virtual Pointer       AllocatePages(usize pageCount) override;
    virtual void          FreePages(Pointer page, usize pageCount) override;

  private:
    Bitmap  m_PageBitmap;

    Pointer m_MemoryTop        = 0;
    Pointer m_UsableMemoryTop  = 0;

    usize   m_PageSize         = 0;
    usize   m_UsableMemorySize = 0;
    usize   m_TotalMemory      = 0;
    usize   m_UsedMemory       = 0;

    Pointer FindFreeRegions(usize& start, usize count, usize limit);
};
