/*
 * Created by v1tr10l7 on 25.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/Device.hpp>

namespace DeviceManager
{
    void Initialize();
};

void Device::Initialize() { DeviceManager::Initialize(); }
