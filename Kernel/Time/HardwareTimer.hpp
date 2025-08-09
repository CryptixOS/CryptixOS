/*
 * Created by v1tr10l7 on 18.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Time.hpp>

enum class TimerMode
{
    eOneShot,
    ePeriodic,
};

class HardwareTimer : public CharacterDevice
{
  public:
    HardwareTimer(StringView name)
        : CharacterDevice(name, ID())
    {
    }
    virtual ~HardwareTimer()               = default;

    virtual StringView ModelString() const = 0;
    virtual usize      InterruptVector() const { return usize(-1); };
    virtual bool       IsCPULocal() const { return false; }

    using OnTickCallback = Delegate<void(struct CPUContext*)>;

    template <void (*Callback)(CPUContext*)>
    inline void SetCallback()
    {
        m_OnTickCallback.Bind<Callback>();
    }

    virtual ErrorOr<void> Start(TimerMode mode, Timestep interval) = 0;
    virtual void          Stop()                                   = 0;

    virtual ErrorOr<void> SetFrequency(usize frequency)            = 0;

    using HookType = IntrusiveRefListHook<HardwareTimer, HardwareTimer*>;
    friend class IntrusiveRefList<HardwareTimer, HookType>;
    friend struct IntrusiveRefListHook<HardwareTimer, HardwareTimer*>;

    using List = IntrusiveRefList<HardwareTimer, HookType>;

  protected:
    OnTickCallback m_OnTickCallback = nullptr;

    HookType       Hook;
};
