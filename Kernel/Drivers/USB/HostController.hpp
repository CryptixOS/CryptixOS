/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Ref.hpp>

namespace USB
{
    class HostController : public RefCounted
    {
      public:
        HostController()                   = default;

        virtual ErrorOr<void> Initialize() = 0;

        virtual ErrorOr<void> Start()      = 0;
        virtual ErrorOr<void> Stop()       = 0;

        virtual ErrorOr<void> Reset()      = 0;

        using HookType = IntrusiveRefListHook<HostController, HostController*>;

        friend class IntrusiveRefList<
            HostController,
            IntrusiveRefListHook<HostController, HostController*>>;
        friend struct IntrusiveRefListHook<HostController, HostController*>;

        using List = IntrusiveRefList<
            HostController,
            IntrusiveRefListHook<HostController, HostController*>>;

      private:
        HookType Hook;
    };
}; // namespace USB
