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
        Spinlock                                 s_DeviceLock;
        Device::List                             s_Devices;

        Spinlock                                 s_CharDeviceLock;
        UnorderedMap<DeviceID, CharacterDevice*> s_CharDevices;
        Spinlock                                 s_BlockDeviceLock;
        UnorderedMap<DeviceID, BlockDevice*>     s_BlockDevices;
    }; // namespace

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
