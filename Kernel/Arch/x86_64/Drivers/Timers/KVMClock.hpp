/*
 * Created by v1tr10l7 on 29.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Time.hpp>

namespace KVM::Clock
{
    ErrorOr<void>     Initialize();
    ErrorOr<Timestep> Read();
}; // namespace KVM::Clock
