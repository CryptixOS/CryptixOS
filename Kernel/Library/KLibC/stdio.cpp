/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdio.h>

#include <Common.hpp>
#include <Prism/Core/Types.hpp>

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

    i32   fputc(i32 c, FILE* stream) { return 0; }
    i32   fputs(const char* s, FILE* stream) { return 0; }
    i32   fputws(const wchar_t* s, FILE* stream) { return -1; }

    i32   fflush(FILE* stream)
    {
        assert(false);
        return -1;
    }

    i32   fprintf(FILE* stream, const char* format, ...) { return -1; }
    usize fwrite(const void* buffer, usize size, usize count, FILE* stream)
    {
        return -1;
    }

    i32 printf(const char* format, ...)
    {
        va_list args;

        va_start(args, format);
        const i32 ret = vprintf(format, args);
        va_end(args);

        return ret;
    }
    i32 vprintf(const char* format, va_list args)
    {
        return Logger::Printv(format, reinterpret_cast<va_list*>(&args));
    }
    i32 sprintf(char* s, const char* format, ...)
    {
        va_list args;

        va_start(args, format);
        const i32 ret = vsprintf(s, format, args);
        va_end(args);

        return ret;
    }
    i32 vsprintf(char* s, const char* format, va_list args)
    {
        return vsnprintf(s, std::numeric_limits<i32>::max(), format, args);
    }
    i32 snprintf(char* s, usize count, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        const i32 ret = vsnprintf(s, count, format, args);
        va_end(args);

        return ret;
    }
    i32 vsnprintf(char* s, usize count, const char* format, va_list args)
    {
        return -1;
    }
    i32 vasprintf(char** s, const char* format, va_list args) { return -1; }
    i32 asprintf(char** s, const char* format, ...) { return -1; }

    i32 putchar(i32 c)
    {
        Logger::LogChar(c);

        return 0;
    }
}
