/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>

#include <Drivers/ACPI/Driver.hpp>
#include <Prism/Core/Error.hpp>

namespace ACPI
{
    class Device;
    namespace Bus
    {
        KERNEL_INIT_SECTION void Initialize();

        ErrorOr<void>            RegisterDriver(Driver* driver);
        void                     UnregisterDriver(Driver* driver);

        ErrorOr<void>            DispatchDriver(Driver* driver);

        ErrorOr<void>            RegisterDevice(ACPI::Device* device);
        void                     UnregisterDevice(ACPI::Device* device);
    }; // namespace Bus
}; // namespace ACPI
