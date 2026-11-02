/*
 * Created by v1tr10l7 on 24.01.2025.
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
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Memory/WeakRef.hpp>

module Cryptix:Impl.Drivers.PCI.Access;

import :Drivers.PCI.Access;
import :Drivers.PCI.Device;
import :Memory.MMIO;
import :Memory.VMM;

namespace PCI
{
    ECAM::ECAM(Pointer address, u8 busStart)
        : m_Base(address)
        , m_BusStart(busStart)
    {
    }

    u32 ECAM::Read(const DeviceAddress& dev, u32 offset, i32 accessSize)
    {
        Pointer addr = GetAddress(dev, offset);

        switch (accessSize)
        {
            case 1: return MMIO::Read<u8>(addr); break;
            case 2: return MMIO::Read<u16>(addr); break;
            case 4: return MMIO::Read<u32>(addr); break;

            default: break;
        }

        assert(false);
        return -1;
    }
    void ECAM::Write(const DeviceAddress& dev, u32 offset, u32 value,
                     i32 accessSize)
    {
        Pointer addr = GetAddress(dev, offset);

        switch (accessSize)
        {
            case 1: MMIO::Write<u8>(addr, value); break;
            case 2: MMIO::Write<u16>(addr, value); break;
            case 4: MMIO::Write<u32>(addr, value); break;

            default: assert(false); break;
        }
    }

    uintptr_t ECAM::GetAddress(const DeviceAddress& dev, u32 offset)
    {
        Pointer phys = m_Base.Raw<u64>() + ((dev.Bus - m_BusStart) << 20)
                     | (dev.Slot << 15) | (dev.Function << 12);

        if (m_Mappings.Contains(phys)) return m_Mappings[phys].Offset(offset);

        usize   size    = 1 << 20;
        // FIXME(v1tr10l7): Research why it doesn't work
        Pointer virt    = phys; // VMM::AllocateSpace(size, sizeof(u32));

        auto    pageMap = VMM::GetKernelPageMap();
        for (usize offset = 0; offset < size; offset += PMM::PAGE_SIZE)
        {
            auto pte = pageMap->Virt2Pte(
                pageMap->TopLevel(), virt.Offset(offset), true, PMM::PAGE_SIZE);

            if (!pte->Address())
                pageMap->Map(virt.Offset(offset), phys.Offset(offset),
                             PageAttributes::eRW);
            else if (!(Arch::VMM::FromNativeFlags(pte->Flags())
                       & PageAttributes::eRW))
                pageMap->Protect(virt.Offset(offset), PageAttributes::eRW);
        }

        m_Mappings[phys] = virt;
        return virt.Offset(offset);
    }
} // namespace PCI
