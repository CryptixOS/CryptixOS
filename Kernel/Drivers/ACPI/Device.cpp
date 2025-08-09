/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/ACPI/Bus.hpp>
#include <Drivers/ACPI/Device.hpp>

namespace ACPI
{
    namespace Bus
    {
        ErrorOr<IrqResource> IrqResourceForHandle(DeviceHandle*);
        ErrorOr<IoResource>  IoResourceForHandle(DeviceHandle*);
    }; // namespace Bus

    Device::Device(DeviceHandle* handle)
    {
        m_IrqResource = Try(Bus::IrqResourceForHandle(handle));
        m_IoResource  = Try(Bus::IoResourceForHandle(handle));
    }
}; // namespace ACPI
