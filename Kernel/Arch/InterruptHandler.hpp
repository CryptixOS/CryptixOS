/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Arch/InterruptManager.hpp>
#include <Prism/Utility/Delegate.hpp>

#include <functional>
#include <optional>

using InterruptRoutine = void (*)(struct CPUContext*);

class InterruptHandler
{
  public:
    bool        eoiFirst = false;

    inline bool IsUsed() const { return m_Routine != nullptr; }
    inline bool IsReserved() const { return m_Reserved; }

    bool        Reserve()
    {
        if (IsReserved()) return false;
        return m_Reserved = true;
    }

    template <typename F>
    inline void SetHandler(F handler)
    {
        m_Routine.BindLambda([handler](CPUContext* ctx) { handler(ctx); });
    }

    inline void SetInterruptVector(u8 interruptVector)
    {
        m_InterruptVector = interruptVector;
    }
    inline u8 GetInterruptVector() const
    {
        return m_InterruptVector.has_value() ? m_InterruptVector.value() : 0;
    }

    operator bool() { return m_Routine; }
    bool operator()(struct CPUContext* ctx) { return HandleInterrupt(ctx); }
    virtual bool HandleInterrupt(CPUContext* ctx)
    {
        if (m_Routine) m_Routine(ctx);
        return true;
    }

  private:
    std::optional<u8>           m_InterruptVector = 0;
    Delegate<void(CPUContext*)> m_Routine         = nullptr;
    bool                        m_Reserved        = false;
};
