/*
 * Created by v1tr10l7 on 21.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Debug/LogSink.hpp>

#include <magic_enum/magic_enum.hpp>
#include <utility>

namespace Prism
{
    constexpr u64 s_LevelForegroundColors[] = {
        FOREGROUND_COLOR_WHITE,  FOREGROUND_COLOR_MAGENTA,
        FOREGROUND_COLOR_GREEN,  FOREGROUND_COLOR_CYAN,
        FOREGROUND_COLOR_YELLOW, FOREGROUND_COLOR_RED,
        FOREGROUND_COLOR_WHITE,
    };

    void LogSink::Log(LogLevel level, std::string_view message)
    {
        std::unique_lock guard(m_Lock);

        PrintLevel(level);
        WriteNoLock(message);

        EndOfLine();
    }

    void LogSink::PutChar(u64 c)
    {
        WriteNoLock(reinterpret_cast<const char*>(c));
    }
    bool LogSink::Enable()
    {
        std::unique_lock guard(m_Lock);

        return std::exchange(m_Enabled, true);
    }
    bool LogSink::Disable()
    {
        std::unique_lock guard(m_Lock);

        return std::exchange(m_Enabled, false);
    }

    void LogSink::PrintLevel(LogLevel level)
    {

        if (level == LogLevel::eNone) return;
        PutChar('[');
        PutChar(s_LevelForegroundColors[std::to_underlying(level)]);
        if (level == LogLevel::eFatal) PutChar(BACKGROUND_COLOR_RED);

        WriteNoLock(magic_enum::enum_name(level).data() + 1);

        PutChar(FOREGROUND_COLOR_WHITE);
        PutChar(BACKGROUND_COLOR_BLACK);
        PutChar(RESET_COLOR);
        WriteNoLock("]: ");
    }
}; // namespace Prism
