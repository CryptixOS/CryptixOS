/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>

export module Cryptix:Library.Logger;

export constexpr usize LOG_SINK_E9              = Bit(0);
export constexpr usize LOG_SINK_SERIAL          = Bit(1);
export constexpr usize LOG_SINK_TERMINAL        = Bit(2);

export constexpr u64   FOREGROUND_COLOR_BLACK   = 0x6d30335b1b;
export constexpr u64   FOREGROUND_COLOR_RED     = 0x6d31335b1b;
export constexpr u64   FOREGROUND_COLOR_GREEN   = 0x6d32335b1b;
export constexpr u64   FOREGROUND_COLOR_YELLOW  = 0x6d33335b1b;
export constexpr u64   FOREGROUND_COLOR_BLUE    = 0x6d34335b1b;
export constexpr u64   FOREGROUND_COLOR_MAGENTA = 0x6d35335b1b;
export constexpr u64   FOREGROUND_COLOR_CYAN    = 0x6d36335b1b;
export constexpr u64   FOREGROUND_COLOR_WHITE   = 0x6d37335b1b;

export constexpr u64   BACKGROUND_COLOR_BLACK   = 0x6d30345b1b;
export constexpr u64   BACKGROUND_COLOR_RED     = 0x6d31345b1b;
export constexpr u64   BACKGROUND_COLOR_GREEN   = 0x6d32345b1b;
export constexpr u64   BACKGROUND_COLOR_YELLOW  = 0x6d33345b1b;
export constexpr u64   BACKGROUND_COLOR_BLUE    = 0x6d34345b1b;
export constexpr u64   BACKGROUND_COLOR_MAGENTA = 0x6d35345b1b;
export constexpr u64   BACKGROUND_COLOR_CYAN    = 0x6d36345b1b;
export constexpr u64   BACKGROUND_COLOR_WHITE   = 0x6d37345b1b;

export constexpr u64   RESET_COLOR              = 0x6d305b1b;

export class VirtualConsole;
export using Prism::LogLevel;
export namespace Logger
{
    CTOS_NO_KASAN void EnableSink(usize sink);
    CTOS_NO_KASAN void DisableSink(usize sink);

    VirtualConsole&          GetTerminal();
    void               Unlock();
} // namespace Logger

