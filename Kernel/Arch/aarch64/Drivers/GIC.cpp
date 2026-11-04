/*
 * Created by v1tr10l7 on 28.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/termios.h>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>

module Cryptix:Impl.Arch.aarch64.Drivers.GIC;

import :Arch.aarch64.Drivers.GIC;
import :Memory.VMM;

DeviceTree::Driver GIC::s_Driver = {
    .Name       = "gic-v1"_sv,
    .Compatible = "arm,cortex-a15-gic"_sv,
    .Probe      = Probe,
};
Ref<GIC> s_GIC = nullptr;

GIC::GIC(DeviceTree::Node& node)
{
    const auto& registers   = node.Registers();
    m_DistributorRegister   = registers[0];
    m_RedistributorRegister = registers[1];
}

bool GIC::Register() { return DeviceTree::RegisterDriver(GIC::s_Driver); }

static constexpr Pointer ExplodeByte(u8 b)
{
    Pointer value = b;
    if constexpr (sizeof(Pointer) == 4) return value.Raw() * 0x01010101;
    else if constexpr (sizeof(Pointer) == 8)
        return value.Raw() * 0x01010101'01010101;
}
ErrorOr<void> GIC::Initialize()
{
    auto version
        = (m_CPUInterface->ID >> CPU_InterfaceRegisters::ID_VERSION_OFFSET)
        & CPU_InterfaceRegisters::ID_VERSION_MASK;
    if (version != 2) return Error(ENOTSUP);
    m_Distributor->Control &= ~DistributorRegisters::IRQ_CONTROL_ENABLE;

    auto irqLineCount
        = (m_Distributor->Type >> DistributorRegisters::IRQ_COUNT_OFFSET)
        & DistributorRegisters::IRQ_COUNT_MASK;
    u32 const maxIrqCount = 32 * (irqLineCount + 1);

    for (usize i = 0; i < maxIrqCount / 32; i++)
    {
        m_Distributor->InterruptClearEnable[i]  = 0xffff'ffff;
        m_Distributor->InterruptClearPending[i] = 0xffff'ffff;
        m_Distributor->InterruptClearActive[i]  = 0xffff'ffff;
    }

    for (usize i = 0; i < maxIrqCount / 4; i++)
    {
        m_Distributor->InterruptPriority[i] = 0;
        m_Distributor->InterruptProcessorTargets[i]
            = ExplodeByte(0xff).Raw<u32>();
    }

    m_CPUInterface->InterruptPriorityMask = 0xff;
    m_CPUInterface->Control |= CPU_InterfaceRegisters::IRQ_CONTROL_ENABLE;
    m_Distributor->Control |= DistributorRegisters::IRQ_CONTROL_ENABLE;

    LogInfo("GIC: Successfully initialized");
    return {};
}
ErrorOr<void> GIC::Shutdown() { return Error(ENOSYS); }

ErrorOr<void> GIC::Mask(u32 irq)
{
    m_Distributor->InterruptClearEnable[irq / 32] = Bit(irq % 32);
    return {};
}
ErrorOr<void> GIC::Unmask(u32 irq)
{
    m_Distributor->InterruptSetEnable[irq / 32] = Bit(irq % 32);
    return {};
}

ErrorOr<void> GIC::SendEOI(u32 irq)
{
    m_CPUInterface->EndOfInterrupt = irq;
    return {};
}

ErrorOr<void> GIC::Probe(DeviceTree::Node& node)
{
    const auto& registers = node.Registers();
    if (registers.Size() < 2) return Error(ENOSYS);

    auto distributorRegister   = registers[0];
    auto redistributorRegister = registers[1];

    LogTrace(
        "GIC: Found generic interrupt controller, register bases =>\n"
        "distributor: {:#x}:{:#x}\n"
        "redistributor: {:#x}:{:#x}\n",
        distributorRegister.Base, distributorRegister.Length,
        redistributorRegister.Base, redistributorRegister.Length);

    auto pageMap = VMM::GetKernelPageMap();
    auto flags   = PageAttributes::eRW | PageAttributes::eWriteThrough;
    pageMap->MapRange(distributorRegister.Base, distributorRegister.Base,
                      distributorRegister.Length, flags);
    pageMap->MapRange(redistributorRegister.Base, redistributorRegister.Base,
                      redistributorRegister.Length, flags);
    s_GIC = CreateRef<GIC>(node);
    if (!s_GIC) return Error(ENOMEM);

    return {};
}
