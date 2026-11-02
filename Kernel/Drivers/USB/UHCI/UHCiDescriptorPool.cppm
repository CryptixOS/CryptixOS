/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/Deque.hpp>

export module Cryptix:Drivers.USB.UHCI.UHCiDescriptorPool;

export import :Memory.PMM;
export import :Memory.VMM;

export namespace USB::UHCI
{
    template <typename T>
    class DescriptorPool : public NonCopyable<DescriptorPool<T>>,
                           public NonMovable<DescriptorPool<T>>
    {
      public:
        void Initialize()
        {
            Pointer pool = PMM::CallocatePages(1);
            m_PoolRegion = pool.ToHigherHalf();

            for (usize i = 0; i < PMM::PAGE_SIZE / sizeof(T); i++)
            {
                Pointer placementAddress = m_PoolRegion.Offset(i * sizeof(T));
                Pointer phys = m_PoolRegion.FromHigherHalf<Pointer>().Offset(
                    (i * sizeof(T)));
                auto* object = new (placementAddress) T(phys);
                m_FreeDescriptorStack.PushBack(
                    object); // Push the descriptor's pointer onto the free list
            }
        }

        T* Allocate()
        {
            ScopedLock guard(m_Lock);
            if (m_FreeDescriptorStack.Empty()) return nullptr;

            T* descriptor = m_FreeDescriptorStack.Front();
            m_FreeDescriptorStack.PopFront();

            return descriptor;
        }
        void Free(T* addr)
        {
            ScopedLock guard(m_Lock);

            m_FreeDescriptorStack.PushBack(addr);
        }

      private:
        Spinlock  m_Lock;
        Pointer   m_PoolRegion = nullptr;
        Deque<T*> m_FreeDescriptorStack;
    };
}; // namespace USB::UHCI
