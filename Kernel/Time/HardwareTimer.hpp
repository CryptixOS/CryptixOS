/*
 * Created by v1tr10l7 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/Utility/Time.hpp>

enum class TimerMode
{
    eOneShot,
    ePeriodic,
};

class HardwareTimer
{
  public:
    HardwareTimer()                           = default;
    virtual ~HardwareTimer()                  = default;

    virtual StringView GetModelString() const = 0;
    virtual usize      InterruptVector() const { return usize(-1); };

    using OnTickCallback = Delegate<void(struct CPUContext*)>;

    template <void (*Callback)(CPUContext*)>
    inline void SetCallback()
    {
        m_OnTickCallback.Bind<Callback>();
    }

    virtual ErrorOr<void> Start(TimerMode mode, Timestep interval) = 0;
    virtual void          Stop()                                   = 0;

    virtual ErrorOr<void> SetFrequency(usize frequency)            = 0;

  protected:
    OnTickCallback m_OnTickCallback = nullptr;
};
