/*
 * Created by v1tr10l7 on 25.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/Core/Concepts.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
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

module Cryptix:Impl.Arch.x86_64.Drivers.PCI;

import :Arch.x86_64.IO;
import :Drivers.PCI.Access;
import :Drivers.PCI.Device;

namespace PCI
{
    constexpr usize CONFIG_ADDRESS = 0x0cf8;
    constexpr usize CONFIG_DATA    = 0x0cfc;
    LegacyAccessMechanism::LegacyAccessMechanism() = default;

    u32 LegacyAccessMechanism::Read(const DeviceAddress& dev, u32 offset,
                                    i32 accessSize)
    {
        u32 address = Bit(31) | (offset & ~0x3) | (dev.Function << 8)
                    | (dev.Slot << 11) | (dev.Bus << 16);
        IO::Out<u32>(CONFIG_ADDRESS, address);

        u32 data = IO::In<u32>(CONFIG_DATA);
        data >>= (offset & 0x3) << 3;

        switch (accessSize)
        {
            case 1: return static_cast<u8>(data);
            case 2: return static_cast<u16>(data);
            case 4: return static_cast<u32>(data);

            default: break;
        }

        LogWarn(
            "PCI::LegacyAccessMechanism::Read: Called with invalid accessSize: "
            "'{}'!",
            accessSize);
        return 0;
    }
    void LegacyAccessMechanism::Write(const DeviceAddress& dev, u32 offset,
                                      u32 value, i32 accessSize)
    {
        u32 address = Bit(31) | (offset & ~3) | (dev.Function << 8)
                    | (dev.Slot << 11) | (dev.Bus << 16);
        IO::Out<u32>(CONFIG_ADDRESS, address);
        u32 oldVal = IO::In<u32>(CONFIG_DATA);
        u32 mask   = 0;

        switch (accessSize)
        {
            case 1: mask = 0xff; break;
            case 2: mask = 0xffff; break;
            case 4: mask = 0xffffffff; break;

            default: break;
        }

        i32 bitOffset = (offset & 0x3) << 3;
        value         = (value & mask) << bitOffset;

        oldVal &= ~(mask << bitOffset);
        oldVal |= value;

        IO::Out<u32>(CONFIG_ADDRESS, address);
        IO::Out<u32>(CONFIG_ADDRESS, oldVal);
    }
}; // namespace PCI
