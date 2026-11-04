/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

export module Cryptix:Drivers.ACPI.Device;

export import :Drivers.ACPI.Resources;

export namespace ACPI
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
