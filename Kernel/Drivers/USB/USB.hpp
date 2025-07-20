/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/USB/HostController.hpp>

namespace USB 
{
    ErrorOr<void> Initialize();
    ErrorOr<void> RegisterController(HostController* controller);
};
