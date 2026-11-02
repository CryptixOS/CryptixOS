/*
 * Created by v1tr10l7 on 25.04.2025.
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
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>

export module Cryptix:Memory.ScopedMapping;

export import :Memory.VMM;

export template <typename T>
class ScopedMapping
{
  public:
    constexpr ScopedMapping(Pointer phys, PageAttributes attributes)
    {
        m_MappingSize    = Math::DivRoundUp(sizeof(T), PMM::PAGE_SIZE);
        m_PageAttributes = attributes;

        m_Structure      = Map(phys, phys, m_MappingSize, attributes);
    }
    constexpr ~ScopedMapping()
    {
        Assert(VMM::UnmapKernelRange(m_Structure, m_MappingSize,
                                     m_PageAttributes));
    }

    constexpr T& operator*() { return *m_Structure; }
    constexpr T* operator->() { return m_Structure; }

  private:
    T*             m_Structure   = nullptr;
    usize          m_MappingSize = 0;
    PageAttributes m_PageAttributes;

    constexpr T*   Map(Pointer virt, Pointer phys, usize size,
                       PageAttributes attributes)
    {
        if (!VMM::MapKernelRange(virt, phys, size, attributes)) return nullptr;

        return virt.As<T>();
    }
};
