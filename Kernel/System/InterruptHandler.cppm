/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <Compiler.hpp>
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
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

export module Cryptix:System.InterruptHandler;

export import :Drivers.InterruptController;
export import :Library.Module;

export struct ExecutionContext;
export enum class IrqResult
{
    eNone    = 0x00,
    eHandled = 0x01,
};
export constexpr usize MAX_IRQ_COUNT = 0x40;

export class Device;
export using InterruptServiceRoutine = Delegate<IrqResult(Device*, ExecutionContext*)>;
export class InterruptDispatcher : public RefCounted
{
  public:
    inline InterruptDispatcher(::Device* device, InterruptController* ctrl,
                               i64 irq)
        : m_Device(device)
        , m_IrqChip(ctrl)
        , m_Irq(irq)
    {
    }

    CTOS_ALWAYS_INLINE ::Device*            Device() const { return m_Device; }
    CTOS_ALWAYS_INLINE InterruptController* Controller() const
    {
        return m_IrqChip;
    }

    CTOS_ALWAYS_INLINE i64 IrqLine() const { return m_Irq; }

    template <typename F>
    inline void SetHandler(F f)
    {
        m_Handler.BindLambda([f](::Device* device, ExecutionContext* ctx) -> IrqResult
                             { return f(device, ctx); });
    }

    inline void       Mask() { m_IrqChip->Mask(m_Irq); }
    inline void       Unmask() { m_IrqChip->Unmask(m_Irq); }

    virtual IrqResult Handle(ExecutionContext* ctx)
    {
        return m_Handler(m_Device, ctx);
    }
    virtual void SendEOI() { m_IrqChip->SendEOI(IrqLine()); }

    using HookType
        = IntrusiveRefListHook<InterruptDispatcher, ::Ref<InterruptDispatcher>>;
    friend class IntrusiveRefList<InterruptDispatcher, HookType>;
    friend struct IntrusiveRefListHook<InterruptDispatcher,
                                       ::Ref<InterruptDispatcher>>;

    using List = IntrusiveRefList<InterruptDispatcher, HookType>;
    HookType Hook;

  protected:
    ::Device*               m_Device  = nullptr;
    InterruptController*    m_IrqChip = nullptr;

    i64                     m_Irq     = -1;
    InterruptServiceRoutine m_Handler;
};
