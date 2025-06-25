/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/CharacterDevice.hpp>
#include <Drivers/DeviceManager.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace DeviceManager
{
    static Spinlock              s_DeviceLock;
    static Device::List          s_Devices;

    static Spinlock              s_CharDeviceLock;
    static CharacterDevice::List s_CharDevices;
    static Vector<Device*>       s_BlockDevices;

    ErrorOr<void>                RegisterCharDevice(CharacterDevice* cdev)
    {
        if (LookupDevice(cdev->ID())) return Error(EEXIST);

        ScopedLock cdevGuard(s_CharDeviceLock);
        s_CharDevices.PushBack(cdev);

        ScopedLock deviceGuard(s_DeviceLock);
        s_Devices.PushBack(cdev);
        DevTmpFs::RegisterDevice(cdev);

        auto path          = "/dev/"_s + cdev->Name();
        // (void)path;
        auto devTmpFsEntry = VFS::MkNod(path, 0666, cdev->ID());
        if (!devTmpFsEntry)
        {
            s_Devices.PopBack();
            s_CharDevices.PopBack();
            return Error(ENXIO);
        }

        return {};
    }
    void RegisterBlockDevice(Device* device)
    {
        s_BlockDevices.PushBack(device);
    }

    Device*          DevicesHead() { return s_Devices.Head(); }
    Device*          DevicesTail() { return s_Devices.Tail(); }

    CharacterDevice* CharDevicesHead() { return s_CharDevices.Head(); }
    CharacterDevice* CharDevicesTail() { return s_CharDevices.Tail(); }

    Device*          LookupDevice(dev_t id)
    {
        auto device = s_Devices.Head();
        while (device && device->ID() != id) device = device->Next();

        return device;
    }
    CharacterDevice* LookupCharDevice(dev_t id)
    {
        auto cdev = s_CharDevices.Head();
        while (cdev && cdev->ID() != id) cdev = cdev->Next();

        return cdev;
    }

    void IterateDevices(DeviceIterator iterator)
    {
        auto device = s_Devices.Head();
        for (; device; device = device->Next())
            if (iterator(device)) break;
    }
    void IterateCharDevices(CharDeviceIterator iterator)
    {
        auto cdev = s_CharDevices.Head();
        for (; cdev; cdev = cdev->Next())
            if (iterator(cdev)) break;
    }

    const Vector<Device*>& GetBlockDevices() { return s_BlockDevices; }
}; // namespace DeviceManager
