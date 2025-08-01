/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <limits.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
    double        strtod(const char* s, char** s_end);
    float         strtof(const char* s, char** s_end);
    long double   strtold(const char* s, char** s_end);
    long int      strtol(const char* nptr, char** endptr, int base);
    long long int strtoll(const char* nptr, char** endptr, int base) throw();
    unsigned long int      strtoul(const char* nptr, char** endptr, int base);
    unsigned long long int strtoull(const char* nptr, char** endptr, int base);

    //--------------------------------------------------------------------------
    // Memory Management
    //--------------------------------------------------------------------------
    void                   free(void* ptr) throw();
    __attribute__((malloc)) __attribute__((alloc_size(1))) void*
                                         malloc(size_t bytes) throw();
    __attribute__((alloc_size(2))) void* realloc(void*  ptr,
                                                 size_t bytes) throw();

    __attribute__((noreturn)) void       abort(void) throw();
#ifdef __cplusplus
};
#endif
