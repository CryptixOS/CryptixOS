/*
 * Created by v1tr10l7 on 31.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Time.hpp>

class ClockSource : public CharacterDevice
{
  public:
    ClockSource(StringView name)
        : CharacterDevice(name, ID())
    {
    }
    virtual StringView        Name() const PM_NOEXCEPT = 0;

    virtual ErrorOr<void>     Enable()                 = 0;
    virtual ErrorOr<void>     Disable()                = 0;

    virtual ErrorOr<Timestep> Now()                    = 0;

    using HookType = IntrusiveRefListHook<ClockSource, ClockSource*>;
    friend class IntrusiveRefList<ClockSource, HookType>;
    friend struct IntrusiveRefListHook<ClockSource, ClockSource*>;

    using List = IntrusiveRefList<ClockSource, HookType>;

  protected:
    static DeviceMajor Major()
    {
        static DeviceMajor major = AllocateMajor().Value();

        return major;
    }
    static DeviceID ID()
    {
        static Atomic<DeviceMinor> nextMinor = 0;
        return MakeDevice(Major(), nextMinor++);
    }

  private:
    HookType Hook;
};
