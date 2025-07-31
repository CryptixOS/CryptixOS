/*
 * Created by v1tr10l7 on 31.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Time.hpp>

class ClockSource
{
  public:
    virtual StringView        Name() const = 0;

    virtual ErrorOr<void>     Enable()     = 0;
    virtual ErrorOr<void>     Disable()    = 0;

    virtual ErrorOr<Timestep> Now()        = 0;

    using HookType = IntrusiveRefListHook<ClockSource, ClockSource*>;
    friend class IntrusiveRefList<ClockSource, HookType>;
    friend struct IntrusiveRefListHook<ClockSource, ClockSource*>;

    using List = IntrusiveRefList<ClockSource, HookType>;

  private:
    HookType Hook;
};
