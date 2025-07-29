/*
 * Created by v1tr10l7 on 25.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Core/DeviceManager.hpp>

CharacterDevice::CharacterDevice(StringView name, dev_t id)
    : Device(name, id)
{
}

CharacterDevice* CharacterDevice::Next() const { return Hook.Next; }
ErrorOr<void>    CharacterDevice::Register(dev_t id, CharacterDevice* device)
{
    return DeviceManager::RegisterCharDevice(device);
}
CharacterDevice* CharacterDevice::Lookup(dev_t deviceID)
{
    return DeviceManager::LookupCharDevice(deviceID);
}

CharacterDevice* CharacterDevice::Head()
{
    return DeviceManager::CharDevicesHead();
}
CharacterDevice* CharacterDevice::Tail()
{
    return DeviceManager::CharDevicesHead();
}

void CharacterDevice::Iterate(CharacterDeviceIterator iterator)
{
    return DeviceManager::IterateCharDevices(iterator);
}
