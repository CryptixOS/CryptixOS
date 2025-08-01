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

    i32   fprintf(FILE* stream, const char* format, ...) { return -1; }
}
