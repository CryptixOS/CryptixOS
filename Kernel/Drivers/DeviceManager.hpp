/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Device.hpp>

#include <Prism/Containers/Vector.hpp>

namespace DeviceManager
{
    void                   RegisterCharDevice(Device* device);
    void                   RegisterBlockDevice(Device* device);

    const Vector<Device*>& GetBlockDevices();
}; // namespace DeviceManager
