/*
 * Created by v1tr10l7 on 21.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

#include <mutex>

namespace Prism
{
    class LogSink
    {
      public:
        void           Log(LogLevel level, std::string_view message);

        void           PutChar(u64 c);

        virtual void   WriteNoLock(std::string_view str) = 0;

        void           EndOfLine() { WriteNoLock("\n"); }

        constexpr bool IsEnabled() const { return m_Enabled; }

        bool           Enable();
        bool           Disable();

      protected:
        std::mutex m_Lock;
        bool       m_Enabled = true;

        void       PrintLevel(LogLevel level);
    };
}; // namespace Prism
