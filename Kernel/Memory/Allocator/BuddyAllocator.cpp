/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/Allocator/BuddyAllocator.hpp>
#include <Prism/Utility/Math.hpp>

ErrorOr<void> BuddyAllocator::Initialize(MemoryMap& memoryMap, usize pageSize)
{
    if (!Math::IsPowerOfTwo(pageSize)) return Error(EINVAL);
    m_PageSize = pageSize;

    return {};
}
void  BuddyAllocator::Shutdown() {}

usize BuddyAllocator::MemoryTop() const { return m_UsableMemoryTop; }
usize BuddyAllocator::TotalMemorySize() const { return m_TotalMemory; }
usize BuddyAllocator::FreeMemorySize() const
{
    return TotalMemorySize() - UsedMemorySize();
}
usize   BuddyAllocator::UsedMemorySize() const { return m_UsedMemory; }
usize   BuddyAllocator::PageSize() const { return m_PageSize; }

Pointer BuddyAllocator::AllocatePages(usize pageCount) { return nullptr; }
void    BuddyAllocator::FreePages(Pointer page, usize pageCount) {}
