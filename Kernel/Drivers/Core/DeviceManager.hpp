/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/stat.h>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Delegate.hpp>

class CharacterDevice;
class BlockDevice;
class Device;
namespace DeviceManager
{
    using CharDeviceIterator  = Delegate<bool(CharacterDevice* cdev)>;
    using BlockDeviceIterator = Delegate<bool(BlockDevice* device)>;

    ErrorOr<void>    RegisterCharDevice(CharacterDevice* device);
    ErrorOr<void>    RegisterBlockDevice(BlockDevice* device);

    CharacterDevice* LookupCharDevice(dev_t id);
    BlockDevice*     LookupBlockDevice(dev_t id);

    void             IterateCharDevices(CharDeviceIterator iterator);
    void             IterateBlockDevices(BlockDeviceIterator iterator);
}; // namespace DeviceManager
