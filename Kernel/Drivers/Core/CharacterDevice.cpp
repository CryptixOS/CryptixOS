/*
 * Created by v1tr10l7 on 25.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/FullDevice.hpp>
#include <Drivers/NullDevice.hpp>
#include <Drivers/ZeroDevice.hpp>

#include <Drivers/Core/CharacterDevice.hpp>
#include <Drivers/Core/DeviceManager.hpp>

#include <Library/Locking/SpinlockProtected.hpp>

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
    };

    IgnoreUnused(DeviceManager::AllocateCharMajor(API::DeviceMajor::MEMORY));
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
