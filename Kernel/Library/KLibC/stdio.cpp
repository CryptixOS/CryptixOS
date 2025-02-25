/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdio.h>

#include <Common.hpp>
#include <Prism/Types.hpp>

using Prism::LogLevel;

namespace std
{
    [[gnu::noreturn]]
    void terminate() noexcept
    {
        Panic("std::terminate()");
    }
} // namespace std

extern "C"
{
    FILE* stdout = (FILE*)&stdout;
    FILE* stderr = (FILE*)stderr;

    int   fputc(int c, FILE* stream)
    {
        /*std::string_view s(reinterpret_cast<const char*>(&c), 1);
        if (stream == stdout) Logger::LogString(s);
        else if (stream == stderr) Logger::Log(LogLevel::eError, s);
        else return -1;
*/
        return 0;
    }
    int fputs(const char* s, FILE* stream)
    {
        /*      if (stream == stdout) Logger::LogString(s);
              else if (stream == stderr) Logger::Log(LogLevel::eError, s);
              else return -1;
      */
        return 0;
    }
    int fputws(const wchar_t* s, FILE* stream) { return -1; }

    int fflush(FILE* stream)
    {
        assert(false);
        return -1;
    }

    int   fprintf(FILE* stream, const char* format, ...) { return -1; }
    usize fwrite(const void* buffer, usize size, usize count, FILE* stream)
    {
        return -1;
    }

    int printf(const char* format, ...)
    {
        va_list args;

        va_start(args, format);
        const int ret = vprintf(format, args);
        va_end(args);

        return ret;
    }
    int vprintf(const char* format, va_list args)
    {
        return Logger::Printv(format, reinterpret_cast<va_list*>(&args));
    }
    int sprintf(char* s, const char* format, ...)
    {
        va_list args;

        va_start(args, format);
        const int ret = vsprintf(s, format, args);
        va_end(args);

        return ret;
    }
    int vsprintf(char* s, const char* format, va_list args)
    {
        return vsnprintf(s, std::numeric_limits<int>::max(), format, args);
    }
    int snprintf(char* s, usize count, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        const int ret = vsnprintf(s, count, format, args);
        va_end(args);

        return ret;
    }
    int vsnprintf(char* s, usize count, const char* format, va_list args)
    {
        return -1;
    }
    int vasprintf(char** s, const char* format, va_list args) { return -1; }
    int asprintf(char** s, const char* format, ...) { return -1; }

    int putchar(int c)
    {
        Logger::LogChar(c);

        return 0;
    }
}
