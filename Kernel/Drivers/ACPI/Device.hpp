/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/ACPI/Resources.hpp>

namespace ACPI
{
    struct DeviceHandle;
    class Device
    {
      public:
        Device(DeviceHandle* handle);

      private:
        IrqResource m_IrqResource;
        IoResource  m_IoResource;
    };
}; // namespace ACPI
