/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/CommandLine.hpp>
#include <Drivers/PCI/HostController.hpp>

#include <Library/Logger.hpp>
#include <Memory/MMIO.hpp>
#include <Memory/VMM.hpp>

namespace PCI
{
    Vector<HostController::IrqRoute> HostController::s_IrqRoutes;

    void                             HostController::InitializeIrqRoutes()
    {
        static const char* rootIds[] = {
            "PNP0A03",
            "PNP0A08",
            nullptr,
        };

        uacpi_find_devices_at(
            uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB),
            rootIds,
            [](void* userData, uacpi_namespace_node* node,
               u32) -> uacpi_iteration_decision
            {
                u64 seg = 0, bus = 0;

                uacpi_eval_integer(node, "_SEG", nullptr, &seg);
                uacpi_eval_integer(node, "_BBN", nullptr, &bus);

                uacpi_resources* resources = nullptr;
                uacpi_get_current_resources(node, &resources);
                uacpi_object_name name = uacpi_namespace_node_name(node);
                LogTrace("PCI: Found pci device using ACPI => {:c}{:c}{:c}{:c}",
                         name.text[0], name.text[1], name.text[2],
                         name.text[3]);

                uacpi_pci_routing_table* pciRoutes;
                auto ret = uacpi_get_pci_routing_table(node, &pciRoutes);

                if (ret != UACPI_STATUS_OK)
                    return UACPI_ITERATION_DECISION_CONTINUE;

                for (usize i = 0; i < pciRoutes->num_entries; i++)
                {
                    auto& entry         = pciRoutes->entries[i];

                    bool  edgeTriggered = false;
                    bool  activeHigh    = false;

                    auto  gsi           = entry.index;

                    i32   slot          = (entry.address >> 16) & 0xffff;
                    i32   func          = entry.address & 0xffff;
                    if (func == 0xffff) func = -1;

                    if (entry.source)
                    {
                        Assert(entry.index == 0);
                        uacpi_resources* resources;
                        ret = uacpi_get_current_resources(entry.source,
                                                          &resources);

                        switch (resources->entries[0].type)
                        {
                            case UACPI_RESOURCE_TYPE_IRQ:
                            {
                                auto* irq = &resources->entries[0].irq;
                                gsi       = irq->irqs[0];

                                if (irq->triggering == UACPI_TRIGGERING_EDGE)
                                    edgeTriggered = true;
                                if (irq->polarity == UACPI_POLARITY_ACTIVE_HIGH)
                                    activeHigh = true;
                                break;
                            }
                            case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
                            {
                                auto* irq = &resources->entries[0].extended_irq;
                                gsi       = irq->irqs[0];

                                if (irq->triggering == UACPI_TRIGGERING_EDGE)
                                    edgeTriggered = true;
                                if (irq->polarity == UACPI_POLARITY_ACTIVE_HIGH)
                                    activeHigh = true;
                                break;
                            }

                            default: Assert(false);
                        }

                        uacpi_free_resources(resources);
                    }

#if 0
                    LogInfo(
                        "PCI: Adding irq entry: {{ gsi: {}, device: {}, "
                        "function: {}, pin: {}, edge: {}, high: {} }}",
                        gsi, slot, func, entry.pin + 1, edgeTriggered,
                        activeHigh);
#endif
                    s_IrqRoutes.EmplaceBack(gsi, slot, func,
                                            static_cast<u8>(entry.pin + 1),
                                            edgeTriggered, activeHigh);
                }

                uacpi_free_pci_routing_table(pciRoutes);
                return UACPI_ITERATION_DECISION_CONTINUE;
            },
            this);
    }
    bool HostController::EnumerateDevices(Enumerator enumerator)
    {
        return EnumerateRootBus(enumerator);
    }

    void HostController::Initialize()
    {
        bool pciUseEcam = CommandLine::GetBoolean("pci.ecam").value_or(true);

        if (pciUseEcam && m_Address)
        {
            LogTrace("PCI: Using the ECAM");
            m_AccessMechanism = new ECAM(m_Address, m_Domain.BusStart);
        }
        else
        {
            LogTrace("PCI: ECAM {}", pciUseEcam ? "Unavailable" : "Disabled");
            m_AccessMechanism = new LegacyAccessMechanism();
        }
        Assert(m_AccessMechanism);

        auto address = PCI::DeviceAddress(m_Domain.ID, 0, 0, 0);
        u8   headerType
            = Read<u8>(address, ToUnderlying(RegisterOffset::eHeaderType));

        if ((headerType & 0x80) == 0)
        {
            Bus* bus = new Bus(this, m_Domain.ID, 0);
            m_RootBuses.PushBack(bus);
            return;
        }
        for (u8 function = 0; function < 8; ++function)
        {
            u16 vendorID = Read<u16>({m_Domain.ID, 0, function, 0},
                                     ToUnderlying(RegisterOffset::eVendorID));
            if (vendorID != PCI_INVALID) break;

            auto bus = new Bus(this, m_Domain.ID, function);
            m_RootBuses.PushBack(bus);
        }
    }

    bool HostController::EnumerateRootBus(Enumerator enumerator)
    {
        for (const auto& bus : m_RootBuses)
            if (bus->Enumerate(enumerator)) return true;
        return false;
    }
}; // namespace PCI
