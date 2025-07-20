/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Memory/Ref.hpp>

namespace USB 
{
    class HostController : public RefCounted
    {
      public:
        HostController() = default;

        virtual ErrorOr<void> Initialize() = 0;

        virtual ErrorOr<void> Start() = 0;
        virtual ErrorOr<void> Stop() = 0;

        virtual ErrorOr<void> Reset() = 0;
        
        friend class IntrusiveList<HostController>;
        friend struct IntrusiveListHook<HostController>;

        using List = IntrusiveList<HostController>;

      private:
        IntrusiveListHook<HostController> Hook;
    };
};
