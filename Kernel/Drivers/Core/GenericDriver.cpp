/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/GenericDriver.hpp>

static Vector<GenericDriver*> s_GenericDrivers;

bool                          RegisterGenericDriver(GenericDriver& driver)
{
    if (!driver.Probe) return false;
    s_GenericDrivers.PushBack(&driver);

    // TODO(v1tr10l7): Drivers are for now dispatched immediately after
    // registration
    return driver.Probe().HasValue();
}
