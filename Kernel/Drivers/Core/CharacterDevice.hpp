/*
 * Created by v1tr10l7 on 20.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/Device.hpp>
#include <Prism/Utility/Delegate.hpp>

class CharacterDevice : public Device
{
  public:
    CharacterDevice(StringView name, dev_t id);

    inline constexpr dev_t           ID() const { return m_ID; }
    CharacterDevice*                 Next() const;

    static ErrorOr<CharacterDevice*> Allocate(StringView  name,
                                              DeviceMajor major,
                                              DeviceMinor minorBase = 0,
                                              usize       count     = 1);

    static ErrorOr<void>             RegisterBaseMemoryDevices();
    static ErrorOr<void>    Register(dev_t id, CharacterDevice* device);
    static CharacterDevice* Lookup(dev_t deviceID);

    static CharacterDevice* Head();
    static CharacterDevice* Tail();

    using CharacterDeviceIterator = Delegate<bool(CharacterDevice* cdev)>;
    static void Iterate(CharacterDeviceIterator iterator);

    using List = IntrusiveList<CharacterDevice>;

  private:
    friend class IntrusiveList<CharacterDevice>;
    friend struct IntrusiveListHook<CharacterDevice>;

    IntrusiveListHook<CharacterDevice> Hook;
};
