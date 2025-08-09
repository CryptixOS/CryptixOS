/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/BlockDevice.hpp>
#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Core/DeviceManager.hpp>

#include <Library/Locking/SpinlockProtected.hpp>

#include <VFS/VFS.hpp>

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
            if (!majors.GetIndex(i)) allocated = i;

        return allocated;
    }
    static Optional<DeviceMajor> FindFreeMajor(Bitmap& majors, DeviceMajor hint)
    {
        usize                 last  = majors.GetSize();
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
