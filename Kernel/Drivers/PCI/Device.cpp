/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Pointer.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/String/StringUtils.hpp>
#endif
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>

module Cryptix:Impl.Drivers.PCI.Device;

#if defined(CTOS_TARGET_X86_64)
import :Arch.x86_64.Drivers.IoApic;
#endif
import :Drivers.PCI.Device;
import :Drivers.PCI.HostController;
import :Drivers.PCI.PCI;
import :Memory.PMM;
import :Memory.VMM;
import :System.InterruptManager;

#ifdef CTOS_TARGET_X86_64
#endif

namespace PCI
{
    Pointer Bar::Map(usize alignment)
    {
        if (!IsMMIO) return nullptr;
        if (Base) return Base;

        if (VMM::GetKernelPageMap()
                ->Virt2Phys(Address.ToHigherHalf<u64>())
                .Raw()
            == u64(-1))
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

    Device::Device(const DeviceAddress& address)
        : m_Address(address)
    {
        m_ID.VendorID          = GetVendorID();
        m_ID.ID                = Read<u16>(Register::eDeviceID);
        m_ID.SubsystemID       = Read<u16>(Register::eSubsystemID);
        m_ID.SubsystemVendorID = Read<u16>(Register::eSubsystemVendorID);
        m_ID.Class             = Read<u8>(Register::eClassID);
        m_ID.Subclass          = Read<u8>(Register::eSubClassID);

        auto status = static_cast<Status>(Read<u16>(Register::eStatus));
        if (!(status & Status::eCapabilityList)) return;

        ReadCapabilities();
        for (const auto& capability : m_Capabilities)
        {
            CapabilityID capabilityID = capability.ID;

            switch (capabilityID)
            {
                case CapabilityID::eMSI:
                    m_MsiSupported = true;
                    m_MsiOffset    = capability.Offset;
                    break;
                case CapabilityID::eMSIx:
                {
                    m_MsixSupported = true;
                    m_MsixOffset    = capability.Offset;
                    MsiXControl control(ReadAt(capability.Offset + 0x02, 2));
                    MsiXAddress table(ReadAt(capability.Offset + 0x04, 4));
                    MsiXAddress pending(ReadAt(capability.Offset + 0x08, 4));

                    usize       count = control.Irqs;
                    m_MsixMessages    = count;
                    m_MsixIrqs.Allocate(count);

                    m_MsixTableBar      = table.Bir;
                    m_MsixTableOffset   = table.Offset << 3;

                    m_MsixPendingBar    = pending.Bir;
                    m_MsixPendingOffset = pending.Offset << 3;
                    break;
                }

                default: break;
            }
        }
    }

    ErrorOr<void> Device::Enable()
    {
        // SetPowerState(PowerState::eD0);
        return {};
    }

    void Device::EnableIOSpace() { SendCommand(Command::eEnableIOSpace, true); }
    void Device::DisableIOSpace()
    {
        SendCommand(Command::eEnableIOSpace, false);
    }

    void Device::EnableMemorySpace()
    {
        SendCommand(Command::eEnableMMIO, true);
    }
    void Device::DisableMemorySpace()
    {
        SendCommand(Command::eEnableMMIO, false);
    }

    void Device::EnableBusMastering()
    {
        EnableIOSpace();
        SendCommand(Command::eEnableBusMaster, true);
    }
    void Device::DisableBusMastering()
    {
        SendCommand(Command::eEnableBusMaster, false);
    }

    void Device::EnableInterrupts()
    {
        SendCommand(Command::eInterruptDisable, false);
    }
    void Device::DisableInterrupts()
    {
        SendCommand(Command::eInterruptDisable, true);
    }

    PowerState    Device::PowerState() const { return PowerState::eD0; }
    ErrorOr<void> Device::SetPowerState(enum PowerState state)
    {
        if (ToUnderlying(state) >= ToUnderlying(PowerState::eD3))
            state = PowerState::eD3;

        if (ToUnderlying(state) > ToUnderlying(PowerState::eD0))
            return Error(EINVAL);
        else if (state == PowerState()) return {};

        auto maybeCapability = FindCapability(CapabilityID::ePowerManagement);
        if (!maybeCapability) return Error(EIO);

        auto capability = maybeCapability.Value();
        if (state == PowerState::eD1 || state == PowerState::eD2)
        {
            u16 pmCapabilities = ReadAt(capability.Offset, 2);
            if (state == PowerState::eD1 && !(pmCapabilities & 0))
                return Error(EIO);
            // else if (state == PowerState::eD2 && (!pmCapabilities & 1))
            //     return Error(EIO);
        }

        // if (ToUnderlying(PowerState()) >= ToUnderlying(PowerState::eD3))

        return Error(ENOSYS);
    }

    Optional<Capability> Device::FindCapability(CapabilityID id)
    {
        for (const auto& capability : m_Capabilities)
            if (capability.ID == id) return capability;

        return NullOpt;
    }

    bool Device::MatchID(Span<DeviceID> idTable, DeviceID& outID)
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
        Ref<InterruptDispatcher> handler
            = InterruptManager::AllocateHandler(0x50, nullptr, "cryptix");
        handler->SetHandler(
            [this](::Device*, ExecutionContext*) -> IrqResult
            {
                if (m_OnIrq) m_OnIrq.Invoke();
                return IrqResult::eHandled;
            });

        m_OnIrq = callback;
        if (MsiXSet(cpuid, handler->IrqLine() + 0x20, -1)) return true;
        if (MsiSet(cpuid, handler->IrqLine() + 0x20, -1)) return true;
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

        u8 vector = handler->IrqLine() + 0x20;
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

    void Device::ReadCapabilities()
    {
        isize ttl = 48;

        for (auto offset = Read<u8>(Register::eCapabilitiesPointer);
             offset >= 0x40 && ttl-- > 0;)
        {
            u16  header       = ReadAt(offset, 2);
            auto capabilityID = static_cast<CapabilityID>(header & 0xff);
            m_Capabilities.EmplaceBack(capabilityID, offset);

            offset = (header >> 8) & 0xfc;
        }
    }
}; // namespace PCI
