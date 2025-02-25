/*
 * Created by v1tr10l7 on 25.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Config.hpp>

namespace Prism
{
    template <typename T>
    class NonMovable
    {
      protected:
        NonMovable()          = default;
        virtual ~NonMovable() = default;

      private:
        NonMovable(NonMovable&&)            = delete;
        NonMovable& operator=(NonMovable&&) = delete;
    };
}; // namespace Prism
