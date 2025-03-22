/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/HostController.hpp>

#include <Library/Logger.hpp>
#include <Memory/MMIO.hpp>
#include <Memory/VMM.hpp>

namespace PCI
{
    std::vector<HostController::IrqRoute> HostController::s_IrqRoutes;

    void                                  HostController::InitializeIrqRoutes()
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
                    s_IrqRoutes.emplace_back(gsi, slot, func,
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
        if (m_Address)
        {
            m_AccessMechanism = new ECAM(m_Address, m_Domain.BusStart);
            return;
        }

        m_AccessMechanism = new LegacyAccessMechanism();
        Assert(m_AccessMechanism);
    }

    bool HostController::EnumerateRootBus(Enumerator enumerator)
    {
        if ((Read<u8>(DeviceAddress(),
                      std::to_underlying(RegisterOffset::eHeaderType))
             & Bit(7))
            == 0)
        {
            if (EnumerateBus(0, enumerator)) return true;
            return false;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                if (Read<u16>({0, 0, i},
                              std::to_underlying(RegisterOffset::eVendorID))
                    == PCI_INVALID)
                    continue;
                if (EnumerateBus(i, enumerator)) return true;
            }
        }

        return false;
    }
    bool HostController::EnumerateBus(u8 bus, Enumerator enumerator)
    {
        for (u8 slot = 0; slot < 32; ++slot)
            if (EnumerateSlot(bus, slot, enumerator)) return true;

        return false;
    }
    bool HostController::EnumerateSlot(const u8 bus, const u8 slot,
                                       Enumerator enumerator)
    {
        DeviceAddress address{};
        address.Domain   = m_Domain.ID;
        address.Bus      = bus;
        address.Slot     = slot;
        address.Function = 0;

        u16 vendorID
            = Read<u16>(address, std::to_underlying(RegisterOffset::eVendorID));
        if (vendorID == PCI_INVALID) return false;

        u8 headerType = Read<u8>(
            address, std::to_underlying(RegisterOffset::eHeaderType));
        if (!(headerType & Bit(7)))
        {
            if (EnumerateFunction(address, enumerator)) return true;
        }
        else
        {
            for (u8 i = 0; i < 8; ++i)
            {
                address.Function = i;
                if (Read<u16>(address,
                              std::to_underlying(RegisterOffset::eVendorID))
                    != PCI_INVALID)
                {
                    if (EnumerateFunction(address, enumerator)) return true;
                }
            }
        }

        return false;
    }
    bool HostController::EnumerateFunction(const DeviceAddress& address,
                                           Enumerator           enumerator)
    {
        if (enumerator(address)) return true;
        u16 type
            = Read<u8>(address, std::to_underlying(RegisterOffset::eClassID))
           << 8;
        type |= Read<u8>(address,
                         std::to_underlying(RegisterOffset::eSubClassID));

        u8 headerType
            = Read<u8>(address, std::to_underlying(RegisterOffset::eHeaderType))
            & 0x7f;
        (void)type;
        if (headerType == 0x01)
        {
            u8 secondaryBus = Read<u8>(
                address, std::to_underlying(RegisterOffset::eSecondaryBus));

            if (EnumerateBus(secondaryBus, enumerator)) return true;
        }
        return false;
    }
}; // namespace PCI
