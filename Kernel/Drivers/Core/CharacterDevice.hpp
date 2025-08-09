/*
 * Created by v1tr10l7 on 20.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Delegate.hpp>

class CharacterDevice : public Device
{
  public:
    CharacterDevice(StringView name, DeviceID id);
    CharacterDevice(StringView name, DeviceMajor major, DeviceMinor minor);

    inline constexpr DeviceID        ID() const { return m_ID; }

    virtual bool                     IsCharDevice() const { return true; }

    static ErrorOr<CharacterDevice*> Allocate(StringView  name,
                                              DeviceMajor major,
                                              DeviceMinor minorBase = 0,
                                              usize       count     = 1);

    static ErrorOr<void>             RegisterBaseMemoryDevices();
    static ErrorOr<void>    Register(DeviceID id, CharacterDevice* device);
    static CharacterDevice* Lookup(DeviceID deviceID);

    using CharacterDeviceIterator = Delegate<bool(CharacterDevice* cdev)>;
    static void Iterate(CharacterDeviceIterator iterator);
};
