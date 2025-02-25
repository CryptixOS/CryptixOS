/*
 * Created by v1tr10l7 on 21.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Types.hpp>

#include <string_view>

namespace Prism
{
    enum class LogLevel
    {
        eNone,
        eDebug,
        eTrace,
        eInfo,
        eWarn,
        eError,
        eFatal,
    };

    namespace Log
    {
        void Print(LogLevel logLevel, std::string_view str);

        template <typename... Args>
        inline void Print(LogLevel logLevel, fmt::format_string<Args...> format,
                          Args&&... args)
        {
            Print(logLevel, fmt::format(format, std::forward<Args>(args)...));
        }

        template <typename... Args>
        inline void Message(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eNone, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Debug(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eDebug, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Trace(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eTrace, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Info(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eInfo, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Warn(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eWarn, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Error(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eError, format, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void Fatal(fmt::format_string<Args...> format, Args&&... args)
        {
            Print(LogLevel::eFatal, format, std::forward<Args>(args)...);
        }
    }; // namespace Log
};     // namespace Prism

#ifdef PRISM_BUILD_DEBUG
    #define PrismDebug(...)        Log::Debug(__VA_ARGS__)
    #define PrismDebugNoAlloc(...) Log::Printf(LogLevel::eDebug, __VA_ARGS__)
#else
    #define PrismDebug(...)
    #define PrismDebugNoAlloc(...)
#endif

#define PrismMessage(...)        Log::Message(__VA_ARGS__)
#define PrismMessageNoAlloc(...) Log::Printf(LogLevel::eNone, __VA_ARGS__)

#if PRISM_LOG_ENABLE == true
    #define PrismTraceNoAlloc(...) Log::Logf(LogLevel::eTrace, __VA_ARGS__)
    #define PrismInfoNoAlloc(...)  Log::Logf(LogLevel::eInfo, __VA_ARGS__)
    #define PrismWarnNoAlloc(...)  Log::Logf(LogLevel::eWarn, __VA_ARGS__)
    #define PrismErrorNoAlloc(...) Log::Logf(LogLevel::eError, __VA_ARGS__)
    #define PrismFatalNoAlloc(...) Log::Logf(LogLevel::eFatal, __VA_ARGS__)

    #define PrismTrace(...)        Log::Trace(__VA_ARGS__)
    #define PrismInfo(...)         Log::Info(__VA_ARGS__)
    #define PrismWarn(...)         Log::Warn(__VA_ARGS__)
    #define PrismError(...)        Log::Error(__VA_ARGS__)
    #define PrismFatal(...)        Log::Fatal(__VA_ARGS__)
#else
    #define PrismTraceNoAlloc(...)
    #define PrismInfoNoAlloc(...)
    #define PrismWarnNoAlloc(...)
    #define PrismErrorNoAlloc(...)
    #define PrismFatalNoAlloc(...)

    #define PrismTrace(...)
    #define PrismInfo(...)
    #define PrismWarn(...)
    #define PrismError(...)
    #define PrismFatal(...)
#endif
