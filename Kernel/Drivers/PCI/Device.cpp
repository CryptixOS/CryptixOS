/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>

#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/Drivers/IoApic.hpp>
#endif

#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>

namespace PCI
{
    Pointer Bar::Map(usize alignment)
    {
        if (!IsMMIO) return nullptr;
        if (Base) return Base;

        if (VMM::GetKernelPageMap()->Virt2Phys(Address.ToHigherHalf<u64>())
            == uintptr_t(-1))
        {
            usize   pageSize = PMM::PAGE_SIZE;
            Pointer base     = Math::AlignDown(Address.Raw(), pageSize);
            auto    len = Math::AlignUp(Address.Offset<u64>(Size), pageSize)
                     - base.Raw();

            auto virt
                = VMM::AllocateSpace(len, alignment ?: pageSize, !Is64Bit);
            VMM::GetKernelPageMap()->MapRange(virt, base, len,
                                              PageAttributes::eRW);
            Base = virt + Address - base;
            return Base;
        }

        return (Base = Address.ToHigherHalf<Pointer>());
    }

    void Device::EnableMemorySpace()
    {
        ScopedLock guard(m_Lock);

        u16        cmd = Read<u16>(RegisterOffset::eCommand) | Bit(0);
        Write<u16>(RegisterOffset::eCommand, cmd);
    }
    void Device::EnableBusMastering()
    {
        ScopedLock guard(m_Lock);
        u16        value = Read<u16>(RegisterOffset::eCommand);
        value |= Bit(2);
        value |= Bit(0);

        Write<u16>(RegisterOffset::eCommand, value);
    }

    bool Device::MatchID(std::span<DeviceID> idTable, DeviceID& outID)
    {
        for (DeviceID& id : idTable)
        {
            if (id.VendorID != DeviceID::ANY_ID && id.VendorID != m_ID.VendorID)
                continue;
            if (id.ID != DeviceID::ANY_ID && id.ID != m_ID.ID) continue;
            if (id.SubsystemID != DeviceID::ANY_ID
                && id.SubsystemID != m_ID.SubsystemID)
                continue;
            if (id.SubsystemVendorID != DeviceID::ANY_ID
                && id.SubsystemVendorID != m_ID.SubsystemVendorID)
                continue;
            if (id.Class != DeviceID::ANY_ID && id.Class != m_ID.Class)
                continue;
            if (id.Subclass != DeviceID::ANY_ID && id.Subclass != m_ID.Subclass)
                continue;

            outID = id;
            return true;
        }

        return false;
    }

    Bar Device::GetBar(u8 index)
    {
        Bar bar = {0};

        if (index > 5) return bar;

        u16 offset  = 0x10 + index * sizeof(u32);
        u32 baseLow = ReadAt(offset, 4);
        WriteAt(offset, 0xffffffff, 4);
        u32 sizeLow = ReadAt(offset, 4);
        WriteAt(offset, baseLow, 4);

        if (baseLow & 1)
        {
            bar.Base    = baseLow & ~0b11;
            bar.Address = bar.Base;
            bar.Size    = (~(sizeLow & ~0b11) + 1) & 0xffff;
            bar.IsMMIO  = false;
        }
        else
        {
            i32       type   = (baseLow >> 1) & 3;
            uintptr_t addr   = 0;
            usize     length = 0;

            switch (type)
            {
                case 0x00:
                    length = ~(sizeLow & ~0b1111) + 1;
                    addr   = baseLow & ~0b1111;
                    break;
                case 0x02:
                    auto offsetH = offset + sizeof(u32);
                    auto barH    = ReadAt(offsetH, 4);

                    WriteAt(offsetH, 0xffffffff, 4);
                    auto sizeHigh = ReadAt(offsetH, 4);
                    WriteAt(offsetH, barH, 4);

                    length = ~((u64(sizeHigh) << 32) | (sizeLow & ~0b1111)) + 1;
                    addr   = (u64(barH) << 32) | (bar & ~0b1111);

                    bar.Is64Bit = true;
                    break;
            }

            bar.Base    = Pointer(0);
            bar.Address = addr;
            bar.Size    = length;
            bar.IsMMIO  = true;

            bar.Size    = ~(sizeLow & ~0b1111) + 1;
            bar.IsMMIO  = true;
        }

        return bar;
    }

    bool Device::RegisterIrq(u64 cpuid, Delegate<void()> callback)
    {
        auto handler = InterruptManager::AllocateHandler();
        handler->Reserve();
        handler->SetHandler(
            [this](CPUContext*)
            {
                if (m_OnIrq) m_OnIrq.Invoke();
            });

        m_OnIrq = callback;
        if (MsiXSet(cpuid, handler->GetInterruptVector(), -1)) return true;
        if (MsiSet(cpuid, handler->GetInterruptVector(), -1)) return true;
#ifdef CTOS_TARGET_X86_64

        auto  pin        = Read<u8>(RegisterOffset::eInterruptPin);
        auto* controller = GetHostController(m_Address.Domain);
        auto  route      = controller->FindIrqRoute(m_Address.Slot, pin);

        if (!route)
        {
            LogError("PCI::Device: Failed to find gsi route for pin: {}", pin);
            return false;
        }

        u16 flags = 0;
        if (!route->ActiveHigh) flags |= Bit(1);
        if (!route->EdgeTriggered) flags |= Bit(3);

        u8 vector = handler->GetInterruptVector();
        IoApic::SetGsiRedirect(cpuid, vector, route->Gsi, flags, true);
        return true;
#endif

        return false;
    }

    bool Device::MsiSet(u64 cpuid, u16 vector, u16 index)
    {
        if (!m_MsiSupported) return false;

        if (index == u16(-1)) index = 0;
        MsiControl control;
        u16*       dest = reinterpret_cast<u16*>(&control);
        *dest           = ReadAt(m_MsiOffset + 0x02, 2);
        Assert((1 << control.Mmc) < 32);

        MsiData    data;
        MsiAddress address;
        data.Vector           = vector;
        data.DeliveryMode     = 0;

        address.BaseAddress   = 0xfee;
        address.DestinationID = cpuid;

        dest                  = reinterpret_cast<u16*>(&address);
        WriteAt(m_MsiOffset + 0x04, *dest, 2);
        dest = reinterpret_cast<u16*>(&data);
        WriteAt(m_MsiOffset + (control.C64 ? 0x0c : 0x08), *dest, 2);

        control.MsiE = 1;
        control.Mme  = 0b000;

        dest         = reinterpret_cast<u16*>(&control);
        WriteAt(m_MsiOffset + 0x02, *dest, 2);

        return true;
    }
    bool Device::MsiXSet(u64 cpuid, u16 vector, u16 index)
    {
        // TODO(v1tr10l7): MsiX
        return false;
    }

    u32 Device::ReadAt(u32 offset, i32 accessSize) const
    {
        auto* controller = GetHostController(m_Address.Domain);
        return controller->Read(m_Address, offset, accessSize);
    }
    void Device::WriteAt(u32 offset, u32 value, i32 accessSize) const
    {
        auto* controller = GetHostController(m_Address.Domain);
        controller->Write(m_Address, offset, value, accessSize);
    }
}; // namespace PCI
