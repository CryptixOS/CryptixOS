/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
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
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:Drivers.Core.Device;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :Library.UserBuffer;
export import :VFS.File;

export using DeviceMajor = u32;
export using DeviceMinor = u32;

export constexpr DeviceID MakeDevice(DeviceMajor major, DeviceMinor minor)
{
    DeviceID dev = DeviceID(major & 0x00000fffu) << 8;
    dev |= DeviceID(major & 0xfffff000u) << 32;
    dev |= DeviceID(minor & 0x000000ffu);
    dev |= DeviceID(minor & 0xffffff00u) << 12;

    return dev;
}
export constexpr DeviceMajor GetDeviceMajor(DeviceID dev)
{
    u32 major = (dev & static_cast<DeviceID>(0x0000'0000'000f'ff00u)) >> 8;
    major |= (dev & static_cast<DeviceID>(0xffff'f000'0000'0000u)) >> 32;

    return major;
}
export constexpr DeviceMinor GetDeviceMinor(DeviceID dev)
{
    u32 minor = dev & static_cast<DeviceID>(0x0000'0000'0000'00ffu);
    minor |= (dev & static_cast<DeviceID>(0x0000'0fff'fff0'0000u)) >> 12;

    return minor;
}

export class Device : public File
{
  public:
    constexpr Device(StringView name, DeviceID id)
        : m_Name(name)
        , m_ID(id)
    {
    }
    Device(StringView name, DeviceMajor major, DeviceMinor minor)
        : m_Name(name)
        , m_ID(MakeDevice(major, minor))
    {
    }
    Device(DeviceMajor major, DeviceMinor minor)
        : m_ID(MakeDevice(major, minor))
    {
    }

    virtual ~Device() = default;

    constexpr inline DeviceID ID() const noexcept { return m_ID; }
    virtual StringView        Name() const noexcept { return m_Name; };

    virtual const stat&       Stats() { return m_Stats; }

    virtual ErrorOr<isize>    IoCtl(usize request, upointer argp)
    {
        return Error(ENOSYS);
    };

    static void Initialize();

    Device*     Next() const { return Hook.Next; }

    using HookType = IntrusiveRefListHook<Device, Device*>;
    using List     = IntrusiveRefList<Device, HookType>;

  protected:
    StringView m_Name = ""_sv;
    DeviceID   m_ID;
    stat       m_Stats;

  private:
    friend class IntrusiveRefList<Device, HookType>;
    friend struct IntrusiveRefListHook<Device, Device*>;

    HookType Hook;
};
