/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

#include "Arch/InterruptManager.hpp"

#include <functional>
#include <optional>

using InterruptRoutine = void (*)(struct CPUContext*);

class InterruptHandler
{
  public:
    inline bool IsUsed() const { return routine != nullptr; }
    inline bool IsReserved() const { return reserved; }

    bool        Reserve()
    {
        if (IsReserved()) return false;
        return reserved = true;
    }

    inline void SetHandler(InterruptRoutine handler) { routine = handler; }

    inline void SetInterruptVector(u8 interruptVector)
    {
        this->interruptVector = interruptVector;
    }
    inline u8 GetInterruptVector() const
    {
        return interruptVector.has_value() ? interruptVector.value() : 0;
    }

    operator bool() { return routine; }
    bool operator()(struct CPUContext* ctx) { return HandleInterrupt(ctx); }
    virtual bool HandleInterrupt(CPUContext* ctx)
    {
        if (routine) routine(ctx);
        return true;
    }

    inline void Register()
    {
        InterruptManager::RegisterInterruptHandler(*this);
    }

  private:
    std::optional<u8> interruptVector = 0;
    InterruptRoutine  routine         = nullptr;
    bool              reserved        = false;
};
