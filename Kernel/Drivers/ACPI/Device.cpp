/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/ACPI/Bus.hpp>
#include <Drivers/ACPI/Device.hpp>
#include <Library/Logger.hpp>

namespace ACPI
{
    namespace Bus
    {
        ErrorOr<IrqResource> IrqResourceForHandle(DeviceHandle*);
        ErrorOr<IoResource>  IoResourceForHandle(DeviceHandle*);
    }; // namespace Bus

    Device::Device(DeviceHandle* handle)
    {
        Memory::Fill(&m_IoResource, 0, sizeof(m_IoResource));
        Memory::Fill(&m_IrqResource, 0, sizeof(m_IrqResource));

        m_IrqResource = TryAcquire(Bus::IrqResourceForHandle(handle));
        m_IoResource  = TryAcquire(Bus::IoResourceForHandle(handle));

        if (!m_IrqResource.IRQs.Empty())
        {
            LogTrace("ACPI: Retrieved the irq resource for device");
            LogTrace("ACPI: Triggering => {}", m_IrqResource.Triggering);
            LogTrace("ACPI: Polarity => {}", m_IrqResource.Polarity);
            LogTrace("ACPI: Sharing => {}", m_IrqResource.Sharing);
            LogTrace("ACPI: WakeCapability => {}",
                     m_IrqResource.WakeCapability);
        }
        for (usize i = 0; usize irq : m_IrqResource.IRQs)
            LogTrace("ACPI: IRQ[{}] => {}", i++, irq);

        if (m_IoResource.Least == 0 && m_IoResource.Highest == 0) return;
        LogTrace("ACPI: Retrieved the io resources for device");

        LogTrace("ACPI: Least IO port => {:#x}", m_IoResource.Least);
        LogTrace("ACPI: Highest IO port => {:#x}", m_IoResource.Highest);
        LogTrace("ACPI: Alignment => {:#x}", m_IoResource.Alignment);
        LogTrace("ACPI: Length => {:#x}", m_IoResource.Length);
    }
}; // namespace ACPI
