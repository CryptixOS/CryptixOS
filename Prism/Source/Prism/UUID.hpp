/*
 * Created by v1tr10l7 on 27.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

namespace Prism
{
    class UUID
    {
      public:
        constexpr UUID(u64 uuid)
            : m_UUID(uuid)
        {
        }

        constexpr operator bool() const { return m_UUID != 0; }
        constexpr operator u64() const { return m_UUID; }

      private:
        u64 m_UUID = 0;
    };
}; // namespace Prism
