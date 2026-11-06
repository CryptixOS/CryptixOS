/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

// Host stand-in for Kernel/Library/Logger.hpp: prints the format string of
// a log call to stderr, without its arguments
#include <cstdio>

namespace TestShims
{
    template <typename... Args>
    inline void Log(const char* level, const char* format, Args&&...)
    {
        std::fprintf(stderr, "[%s]: %s\n", level, format);
    }
} // namespace TestShims

#define LogTrace(...) TestShims::Log("Trace", __VA_ARGS__)
#define LogDebug(...) TestShims::Log("Debug", __VA_ARGS__)
#define LogInfo(...)  TestShims::Log("Info", __VA_ARGS__)
#define LogWarn(...)  TestShims::Log("Warn", __VA_ARGS__)
#define LogError(...) TestShims::Log("Error", __VA_ARGS__)
#define LogFatal(...) TestShims::Log("Fatal", __VA_ARGS__)
