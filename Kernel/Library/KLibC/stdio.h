/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
    #define restrict
#endif

#define EOF (-1)
    typedef size_t FILE;
    typedef size_t fpos_t;

    //    extern FILE*   stdin;
    extern FILE*   stdout;
    extern FILE*   stderr;
    int fprintf(FILE* restrict stream, const char* restrict format, ...)
        __attribute__((format(printf, 2, 3)));
#ifdef __cplusplus
} // extern "C"
#endif
