/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/DeviceManager.hpp>

namespace DeviceManager
{
    static Vector<Device*> s_CharDevices;
    static Vector<Device*> s_BlockDevices;

    void RegisterCharDevice(Device* device) { s_CharDevices.PushBack(device); }
    void RegisterBlockDevice(Device* device)
    {
        s_BlockDevices.PushBack(device);
    }

    const Vector<Device*>& GetBlockDevices() { return s_BlockDevices; }
}; // namespace DeviceManager
