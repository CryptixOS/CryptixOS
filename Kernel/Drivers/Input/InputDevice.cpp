/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.Drivers.Input.InputDevice;

import :API.DeviceIDs;
import :Drivers.Core.DeviceManager;
import :Drivers.Input.InputDevice;
import :Library.Locking.SpinlockProtected;
import :VFS.VFS;

static Spinlock                             s_Lock;
static Bitmap                               s_AllocatedMinors{};
static Atomic<DeviceMinor>                  s_LastFreeMinor = 0;

static UnorderedMap<DeviceID, InputDevice*> s_InputDevices;

InputDevice::InputDevice(StringView name, DeviceMinor minor)
    : CharacterDevice(name, API::DeviceMajor::INPUT, minor)
{
}

void InputDevice::Initialize()
{
    LogTrace("Input: Creating /dev/input...");
    Assert(VFS::CreateDirectory("/dev/input", 755));

    ScopedLock guard(s_Lock);
    Assert(DeviceManager::AllocateCharMajor(API::DeviceMajor::INPUT));
    s_AllocatedMinors.Allocate(4096);
}
DeviceMinor InputDevice::AllocateMinor()
{
    auto scanArea = [](const Bitmap& bitmap, usize start,
                       usize end) -> Optional<DeviceMinor>
    {
        for (usize i = start; i < end; i++)
            if (bitmap[i]) return i;

        return NullOpt;
    };

    ScopedLock guard(s_Lock);
    auto       minor = scanArea(s_AllocatedMinors, s_LastFreeMinor, 4096);
    if (!minor)
    {
        auto end        = s_LastFreeMinor.Load();
        s_LastFreeMinor = 0;
        minor           = scanArea(s_AllocatedMinors, 0, end);
    }

    if (minor)
    {
        s_AllocatedMinors.SetIndex(*minor, true);
        ++s_LastFreeMinor;
        if (s_LastFreeMinor >= 4095) s_LastFreeMinor = 0;

        return minor;
    }

    return minor;
}
void InputDevice::FreeMinor(DeviceMinor minor)
{
    ScopedLock guard(s_Lock);
    s_AllocatedMinors.SetIndex(minor, false);
}

void InputDevice::Register(InputDevice* input)
{
    Assert(input);

    ScopedLock guard(s_Lock);
    auto       id      = input->ID();

    s_InputDevices[id] = input;
}
void InputDevice::Unregister(InputDevice* input)
{
    Assert(input);

    ScopedLock guard(s_Lock);
    auto       id = input->ID();

    s_InputDevices.Erase(id);
}
