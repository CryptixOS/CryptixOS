/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/stat.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Drivers.Core.DeviceManager;

import :Drivers.Core.BlockDevice;
import :Drivers.Core.CharacterDevice;

export class CharacterDevice;
export class BlockDevice;
export class Device;
export namespace DeviceManager
{
    using CharDeviceIterator  = Delegate<bool(CharacterDevice* cdev)>;
    using BlockDeviceIterator = Delegate<bool(BlockDevice* device)>;

    Optional<u32>    AllocateCharMajor(usize hint);
    Optional<u32>    AllocateBlockMajor(usize hint);

    ErrorOr<void>    RegisterCharDevice(CharacterDevice* device);
    ErrorOr<void>    RegisterBlockDevice(BlockDevice* device);

    CharacterDevice* LookupCharDevice(dev_t id);
    BlockDevice*     LookupBlockDevice(dev_t id);

    void             IterateCharDevices(CharDeviceIterator iterator);
    void             IterateBlockDevices(BlockDeviceIterator iterator);
}; // namespace DeviceManager
