/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/stat.h>

#include <Prism/Core/Error.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Utility/Delegate.hpp>

class CharacterDevice;
class Device;
namespace DeviceManager
{
    using DeviceIterator     = Delegate<bool(Device* device)>;
    using CharDeviceIterator = Delegate<bool(CharacterDevice* cdev)>;

    ErrorOr<void>          RegisterCharDevice(CharacterDevice* device);
    void                   RegisterBlockDevice(Device* device);

    Device*                DevicesHead();
    Device*                DevicesTail();

    CharacterDevice*       CharDevicesHead();
    CharacterDevice*       CharDevicesTail();

    Device*                LookupDevice(dev_t id);
    CharacterDevice*       LookupCharDevice(dev_t id);

    void                   IterateDevices(DeviceIterator iterator);
    void                   IterateCharDevices(CharDeviceIterator iterator);

    const Vector<Device*>& GetBlockDevices();
}; // namespace DeviceManager
