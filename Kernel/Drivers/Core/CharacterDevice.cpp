/*
 * Created by v1tr10l7 on 25.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
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
#include <Prism/Algorithm/Random.hpp>
#include <Prism/Containers/Array.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>

module Cryptix:Impl.Drivers.Core.CharacterDevice;

import :Drivers.FullDevice;
import :Drivers.NullDevice;
import :Drivers.RandomDevice;
import :Drivers.ZeroDevice;
import :Drivers.Core.CharacterDevice;
import :Drivers.Core.DeviceManager;
import :Library.Locking.SpinlockProtected;

static SpinlockProtected<Array<bool, 256>> s_AllocatedMajors{};

CharacterDevice::CharacterDevice(StringView name, DeviceID id)
    : Device(name, id)
{
}
CharacterDevice::CharacterDevice(StringView name, DeviceMajor major,
                                 DeviceMinor minor)
    : Device(name, major, minor)
{
}

ErrorOr<void> CharacterDevice::RegisterBaseMemoryDevices()
{
    CharacterDevice* devices[] = {
        new NullDevice,
        new ZeroDevice,
        new FullDevice,
        new RandomDevice,
    };

    if (auto result
        = DeviceManager::AllocateCharMajor(API::DeviceMajor::MEMORY);
        !result)
    {
        LogError(
            "CharacterDevice: Failed to allocate major for Memory Devices");
        return Error(ENOMEM);
    }
    for (auto device : devices)
    {
        if (!device) return Error(ENOMEM);

        auto result = DeviceManager::RegisterCharDevice(device);
        if (!result)
        {
            delete device;
            return result;
        }
    }

    return {};
}
ErrorOr<void> CharacterDevice::Register(DeviceID id, CharacterDevice* device)
{
    return DeviceManager::RegisterCharDevice(device);
}
CharacterDevice* CharacterDevice::Lookup(DeviceID deviceID)
{
    return DeviceManager::LookupCharDevice(deviceID);
}

void CharacterDevice::Iterate(CharacterDeviceIterator iterator)
{
    return DeviceManager::IterateCharDevices(iterator);
}
