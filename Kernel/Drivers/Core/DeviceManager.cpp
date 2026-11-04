/*
 * Created by v1tr10l7 on 30.03.2025.
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

module Cryptix:Impl.Drivers.Core.DeviceManager;

import :Drivers.Core.BlockDevice;
import :Drivers.Core.CharacterDevice;
import :Drivers.Core.DeviceManager;
import :Library.Locking.SpinlockProtected;
import :VFS.VFS;

namespace DeviceManager
{
    namespace
    {
        Optional<DeviceMajor>                    s_LeastCharMajor = 0;
        Bitmap                                   s_CharMajors{};

        Optional<DeviceMajor>                    s_LeastBlockMajor = 0;
        Bitmap                                   s_BlockMajors{};

        Spinlock                                 s_DeviceLock;
        Device::List                             s_Devices;

        Spinlock                                 s_CharDeviceLock;
        UnorderedMap<DeviceID, CharacterDevice*> s_CharDevices;
        Spinlock                                 s_BlockDeviceLock;
        UnorderedMap<DeviceID, BlockDevice*>     s_BlockDevices;
    }; // namespace

    static Optional<DeviceMajor> FindFreeMajor(Bitmap& majors, isize start,
                                               isize end);
    static Optional<DeviceMajor> FindFreeMajor(Bitmap&     majors,
                                               DeviceMajor hint);

    void                         Initialize()
    {
        s_CharMajors.Allocate(4096);
        s_BlockMajors.Allocate(4096);
    }
    Optional<DeviceMajor> AllocateCharMajor(usize hint)
    {
        Optional<DeviceMajor> allocated = FindFreeMajor(s_CharMajors, hint);
        if (!allocated) return NullOpt;

        if (allocated == s_LeastCharMajor)
            s_LeastCharMajor = FindFreeMajor(s_CharMajors, 0);
        s_CharMajors.SetIndex(allocated.Value(), true);
        return allocated;
    }
    Optional<DeviceMajor> AllocateBlockMajor(usize hint)
    {
        Optional<DeviceMajor> allocated = FindFreeMajor(s_BlockMajors, hint);
        if (!allocated) return NullOpt;

        if (allocated == s_LeastBlockMajor)
            s_LeastBlockMajor = FindFreeMajor(s_BlockMajors, 0);
        s_BlockMajors.SetIndex(allocated.Value(), true);
        return allocated;
    }

    static Optional<DeviceMajor> FindFreeMajor(Bitmap& majors, isize start,
                                               isize end)
    {
        Optional<DeviceMajor> allocated = NullOpt;

        for (isize i = start; i < end; i++)
            if (!majors[i]) allocated = i;

        return allocated;
    }
    static Optional<DeviceMajor> FindFreeMajor(Bitmap& majors, DeviceMajor hint)
    {
        usize                 last  = majors.BitCount();
        Optional<DeviceMajor> major = FindFreeMajor(majors, hint, last);
        if (!major) major = FindFreeMajor(majors, 0, hint);

        return major;
    }

    ErrorOr<void> RegisterCharDevice(CharacterDevice* cdev)
    {
        auto id = cdev->ID();
        if (LookupCharDevice(id)) return Error(EEXIST);

        ScopedLock cdevGuard(s_CharDeviceLock);
        s_CharDevices[id] = cdev;

        ScopedLock deviceGuard(s_DeviceLock);
        s_Devices.PushBack(cdev);

        // NOTE(v1tr10l7): The vfs will create missing nodes during it's
        // initialization
        if (!VFS::IsInitialized()) return {};

        auto path          = "/dev/"_s + cdev->Name();
        auto devTmpFsEntry = VFS::CreateNode(path, S_IFCHR | 0666, cdev->ID());
        if (!devTmpFsEntry)
        {
            s_Devices.PopBack();
            s_CharDevices.Erase(id);
            return Error(ENXIO);
        }

        return {};
    }
    ErrorOr<void> RegisterBlockDevice(BlockDevice* bdev)
    {
        auto id = bdev->ID();
        if (LookupBlockDevice(id)) return Error(EEXIST);

        ScopedLock bdevGuard(s_BlockDeviceLock);
        s_BlockDevices[id] = bdev;

        ScopedLock deviceGuard(s_DeviceLock);
        s_Devices.PushBack(bdev);

        // NOTE(v1tr10l7): The vfs will create missing nodes during it's
        // initialization
        if (!VFS::IsInitialized()) return {};

        auto path          = "/dev/"_s + bdev->Name();
        auto devTmpFsEntry = VFS::CreateNode(path, S_IFBLK | 0666, bdev->ID());
        if (!devTmpFsEntry)
        {
            s_Devices.PopBack();
            s_BlockDevices.Erase(id);
            return Error(ENXIO);
        }

        return {};
    }

    CharacterDevice* LookupCharDevice(DeviceID id)
    {
        auto it = s_CharDevices.Find(id);
        if (it != s_CharDevices.end()) return it->Value;

        return nullptr;
    }
    BlockDevice* LookupBlockDevice(DeviceID id)
    {
        auto it = s_BlockDevices.Find(id);
        if (it != s_BlockDevices.end()) return it->Value;

        return nullptr;
    }

    void IterateCharDevices(CharDeviceIterator iterator)
    {
        for (const auto [id, cdev] : s_CharDevices)
            if (!iterator(cdev)) break;
    }
    void IterateBlockDevices(BlockDeviceIterator iterator)
    {
        for (const auto [id, bdev] : s_BlockDevices)
            if (!iterator(bdev)) break;
    }
}; // namespace DeviceManager
