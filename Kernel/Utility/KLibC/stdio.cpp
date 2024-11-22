/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdio.h>

#include "Common.hpp"
#include "Utility/Types.hpp"

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
    int   fputws(const wchar_t* s, FILE* stream) { return -1; }

    int   fprintf(FILE* stream, const char* format, ...) { return -1; }
    usize fwrite(const void* buffer, usize size, usize count, FILE* stream)
    {
        return -1;
    }

    int printf(const char* format, ...) { return -1; }
    int vprintf(const char* format, va_list args) { return -1; }
    int sprintf(char* s, const char* format, ...) { return -1; }
    int vsprintf(char* s, const char* format, va_list args) { return -1; }
    int snprintf(char* s, usize count, const char* format, ...) { return -1; }
    int vsnprintf(char* s, usize count, const char* format, va_list args)
    {
        return -1;
    }
    int vasprintf(char** s, const char* format, va_list args) { return -1; }
    int asprintf(char** s, const char* format, ...) { return -1; }
}
