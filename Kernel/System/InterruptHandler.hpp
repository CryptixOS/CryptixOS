/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/InterruptController.hpp>
#include <Library/Module.hpp>
#include <Prism/Utility/Delegate.hpp>

struct ExecutionContext;
enum class IrqResult
{
    eNone    = 0x00,
    eHandled = 0x01,
};
constexpr usize MAX_IRQ_COUNT = 0x40;

class Device;
using InterruptServiceRoutine = Delegate<IrqResult(Device*, ExecutionContext*)>;
class InterruptDispatcher : public RefCounted
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
