/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/Allocator/PageFrameAllocator.hpp>
#include <Prism/Containers/Bitmap.hpp>

class BuddyAllocator : public PageFrameAllocator
{
  public:
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
    Pointer m_MemoryTop        = 0;
    Pointer m_UsableMemoryTop  = 0;

    usize   m_PageSize         = 0;
    usize   m_UsableMemorySize = 0;
    usize   m_TotalMemory      = 0;
    usize   m_UsedMemory       = 0;

    Pointer FindFreeRegions(usize& start, usize count, usize limit);
};
