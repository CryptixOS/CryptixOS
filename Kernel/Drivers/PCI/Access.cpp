/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Access.hpp>
#include <Drivers/PCI/Device.hpp>

#include <Memory/MMIO.hpp>
#include <Memory/VMM.hpp>
#include <Prism/Core/Types.hpp>

namespace PCI
{
    u32 ECAM::Read(const DeviceAddress& dev, u32 offset, i32 accessSize)
    {
        Pointer addr = GetAddress(dev, offset);

        switch (accessSize)
        {
            case 1: return MMIO::Read<u8>(addr); break;
            case 2: return MMIO::Read<u16>(addr); break;
            case 4: return MMIO::Read<u32>(addr); break;

            default: break;
        }

        assert(false);
        return -1;
    }
    void ECAM::Write(const DeviceAddress& dev, u32 offset, u32 value,
                     i32 accessSize)
    {
        Pointer addr = GetAddress(dev, offset);

        switch (accessSize)
        {
            case 1: MMIO::Write<u8>(addr, value); break;
            case 2: MMIO::Write<u16>(addr, value); break;
            case 4: MMIO::Write<u32>(addr, value); break;

            default: assert(false); break;
        }
    }

    uintptr_t ECAM::GetAddress(const DeviceAddress& dev, u32 offset)
    {
        Pointer phys = m_Base.Raw<u64>() + ((dev.Bus - m_BusStart) << 20)
                     | (dev.Slot << 15) | (dev.Function << 12);

        if (m_Mappings.Contains(phys)) return m_Mappings[phys].Offset(offset);

        usize   size    = 1 << 20;
        // FIXME(v1tr10l7): Research why it doesn't work
        Pointer virt    = phys; // VMM::AllocateSpace(size, sizeof(u32));

        auto    pageMap = VMM::GetKernelPageMap();
        for (usize offset = 0; offset < size; offset += PMM::PAGE_SIZE)
        {
            auto pte = pageMap->Virt2Pte(
                pageMap->TopLevel(), virt.Offset(offset), true, PMM::PAGE_SIZE);

            if (!pte->Address())
                pageMap->Map(virt.Offset(offset), phys.Offset(offset),
                             PageAttributes::eRW);
            else if (!(Arch::VMM::FromNativeFlags(pte->Flags())
                       & PageAttributes::eRW))
                pageMap->Protect(virt.Offset(offset), PageAttributes::eRW);
        }

        m_Mappings[phys] = virt;
        return virt.Offset(offset);
    }
} // namespace PCI
