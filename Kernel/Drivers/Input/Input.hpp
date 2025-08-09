/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

class InputDevice : public CharacterDevice
{
  public:
    InputDevice(StringView name, DeviceMinor minor);

    static void        Initialize();

    static DeviceMinor AllocateMinor();
    static void        FreeMinor(DeviceMinor minor);

    static void        Register(InputDevice* input);
    static void        Unregister(InputDevice* input);
};
