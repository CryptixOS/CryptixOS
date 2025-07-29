/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>

#include <cstdarg>

constexpr usize LOG_SINK_E9              = Bit(0);
constexpr usize LOG_SINK_SERIAL          = Bit(1);
constexpr usize LOG_SINK_TERMINAL        = Bit(2);

constexpr u64   FOREGROUND_COLOR_BLACK   = 0x6d30335b1b;
constexpr u64   FOREGROUND_COLOR_RED     = 0x6d31335b1b;
constexpr u64   FOREGROUND_COLOR_GREEN   = 0x6d32335b1b;
constexpr u64   FOREGROUND_COLOR_YELLOW  = 0x6d33335b1b;
constexpr u64   FOREGROUND_COLOR_BLUE    = 0x6d34335b1b;
constexpr u64   FOREGROUND_COLOR_MAGENTA = 0x6d35335b1b;
constexpr u64   FOREGROUND_COLOR_CYAN    = 0x6d36335b1b;
constexpr u64   FOREGROUND_COLOR_WHITE   = 0x6d37335b1b;

constexpr u64   BACKGROUND_COLOR_BLACK   = 0x6d30345b1b;
constexpr u64   BACKGROUND_COLOR_RED     = 0x6d31345b1b;
constexpr u64   BACKGROUND_COLOR_GREEN   = 0x6d32345b1b;
constexpr u64   BACKGROUND_COLOR_YELLOW  = 0x6d33345b1b;
constexpr u64   BACKGROUND_COLOR_BLUE    = 0x6d34345b1b;
constexpr u64   BACKGROUND_COLOR_MAGENTA = 0x6d35345b1b;
constexpr u64   BACKGROUND_COLOR_CYAN    = 0x6d36345b1b;
constexpr u64   BACKGROUND_COLOR_WHITE   = 0x6d37345b1b;

constexpr u64   RESET_COLOR              = 0x6d305b1b;

#define CTOS_NO_KASAN __attribute__((no_sanitize("address")))

class Terminal;
using Prism::LogLevel;
namespace Logger
{
    CTOS_NO_KASAN void  EnableSink(usize sink);
    CTOS_NO_KASAN void  DisableSink(usize sink);

    void                LogChar(u64 c);
    CTOS_NO_KASAN isize Print(StringView string);
    CTOS_NO_KASAN isize Printv(const char* format, va_list* args);

    CTOS_NO_KASAN isize Log(LogLevel logLevel, StringView str,
                            bool printNewline = true);
    CTOS_NO_KASAN isize Logf(LogLevel logLevel, const char* format, ...);
    CTOS_NO_KASAN isize Logv(LogLevel logLevel, const char* format,
                             va_list& args, bool printNewline = true);

    Terminal&           GetTerminal();
    void                Unlock();
} // namespace Logger

#define CTOS_BUILD_DEBUG
#ifdef CTOS_BUILD_DEBUG
    #define LogDebug(...)                                                      \
        Logger::Log(LogLevel::eDebug, fmt::format(__VA_ARGS__).data())
    #define EarlyLogDebug(...) Logger::Logf(LogLevel::eDebug, __VA_ARGS__)
#else
    #define LogDebug(...)
    #define EarlyLogDebug(...)
#endif

#define LogMessage(...)                                                        \
    Logger::Log(LogLevel::eNone, fmt::format(__VA_ARGS__).data())
#define EarlyLogMessage(...) Logger::Logf(LogLevel::eNone, __VA_ARGS__)

#define ENABLE_LOGGING       true
#if ENABLE_LOGGING == true
    #define EarlyLogTrace(...) Logger::Logf(LogLevel::eTrace, __VA_ARGS__)
    #define EarlyLogInfo(...)  Logger::Logf(LogLevel::eInfo, __VA_ARGS__)
    #define EarlyLogWarn(...)  Logger::Logf(LogLevel::eWarn, __VA_ARGS__)
    #define EarlyLogError(...) Logger::Logf(LogLevel::eError, __VA_ARGS__)
    #define EarlyLogFatal(...) Logger::Logf(LogLevel::eFatal, __VA_ARGS__)

    #define LogTrace(...)      Prism::Log::Trace(__VA_ARGS__)
    #define LogInfo(...)       Prism::Log::Info(__VA_ARGS__)
    #define LogWarn(...)       Prism::Log::Warn(__VA_ARGS__)
    #define LogError(...)      Prism::Log::Error(__VA_ARGS__)
    #define LogFatal(...)      Prism::Log::Fatal(__VA_ARGS__)
#else
    #define EarlyLogTrace(...)
    #define EarlyLogInfo(...)
    #define EarlyLogWarn(...)
    #define EarlyLogError(...)
    #define EarlyLogFatal(...)

    #define LogTrace(...)
    #define LogInfo(...)
    #define LogWarn(...)
    #define LogError(...)
    #define LogFatal(...)
#endif
