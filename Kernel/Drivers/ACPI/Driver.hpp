/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/String/String.hpp>

namespace ACPI
{
    struct DeviceHandle;
    struct Driver
    {
        String           Name;
        Span<StringView> MatchIDs;

        using HookType = IntrusiveRefListHook<Driver, Driver*>;

        friend class IntrusiveRefList<Driver,
                                      IntrusiveRefListHook<Driver, Driver*>>;
        friend struct IntrusiveRefListHook<Driver, Driver*>;

        using List
            = IntrusiveRefList<Driver, IntrusiveRefListHook<Driver, Driver*>>;

        HookType Hook;

        using ProbeFn  = ErrorOr<void> (*)(DeviceHandle* handle, StringView id);
        using RemoveFn = void (*)(DeviceHandle* handle);

        ProbeFn  Probe;
        RemoveFn Remove;
    };
}; // namespace ACPI
