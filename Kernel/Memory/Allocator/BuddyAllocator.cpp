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

module Cryptix:Impl.Memory.Allocator.BuddyAllocator;

import :Memory.Allocator.BuddyAllocator;

ErrorOr<void> BuddyAllocator::Initialize(MemoryMap& memoryMap, usize pageSize)
{
    if (!Math::IsPowerOfTwo(pageSize)) return Error(EINVAL);
    m_PageSize = pageSize;

    auto memoryZones = Span(memoryMap.Entries, memoryMap.EntryCount);
    for (const auto& zone : memoryZones)
        IgnoreUnused(zone);

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
