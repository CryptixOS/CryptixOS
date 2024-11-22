/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <limits.h>
#include <stddef.h>

// #include_next <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int quotient;
        int reminder;
    } div_t;
    typedef struct
    {
        long quotient;
        long reminder;
    } ldiv_t;
    typedef struct
    {
        long long quotient;
        long long reminder;
    } lldiv_t;

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define RAND_MAX     INT_MAX
// TODO: MB_CUR_MAX
#define MB_CUR_MAX   0

    //--------------------------------------------------------------------------
    // Numeric conversion functions
    //--------------------------------------------------------------------------
    double        atof(const char* s);
    int           atoi(const char* s);
    long int      atol(const char* s);
    long long int atoll(const char* s);
    double        strtod(const char* s, char** s_end);
    float         strtof(const char* s, char** s_end);
    long double   strtold(const char* s, char** s_end);
    long int      strtol(const char* nptr, char** endptr, int base);
    long long int strtoll(const char* nptr, char** endptr, int base) throw();
    unsigned long int      strtoul(const char* nptr, char** endptr, int base);
    unsigned long long int strtoull(const char* nptr, char** endptr, int base);

    //--------------------------------------------------------------------------
    // Pseudo-random sequence generation functions
    //--------------------------------------------------------------------------
    int                    rand(void);
    void                   srand(unsigned int seed);

    //--------------------------------------------------------------------------
    // Memory Management
    //--------------------------------------------------------------------------
    __attribute__((malloc)) __attribute__((alloc_size(1, 2))) void*
         calloc(size_t bytes, size_t count) throw();
    void free(void* ptr) throw();
    __attribute__((malloc)) __attribute__((alloc_size(1))) void*
                                         malloc(size_t bytes) throw();
    __attribute__((alloc_size(2))) void* realloc(void*  ptr,
                                                 size_t bytes) throw();

    //--------------------------------------------------------------------------
    // Communication with the environment
    //--------------------------------------------------------------------------
    __attribute__((noreturn)) void       abort(void) throw();
    int                                  atexit(void (*func)(void)) noexcept;
    __attribute__((noreturn)) void       _Exit(int status) noexcept;
    char*                                getenv(const char* name);
    int                                  system(const char* string);

    //--------------------------------------------------------------------------
    // Searching and sorting utilities
    //--------------------------------------------------------------------------
    void* bsearch(const void* key, const void* base, size_t nmemb, size_t size,
                  int (*compar)(const void*, const void*));
    void  qsort(void* base, size_t nmemb, size_t size,
                int (*compar)(const void*, const void*));

    //--------------------------------------------------------------------------
    // Integer arithmetic functions
    //--------------------------------------------------------------------------
    int   abs(int j);
    long int      labs(long int j);
    long long int llabs(long long int j);
    div_t         div(int numer, int denom);
    ldiv_t        ldiv(long int numer, long int denom);
    lldiv_t       lldiv(long long int numer, long long int denom);

    //--------------------------------------------------------------------------
    // Multibyte/wide character conversion functions
    //--------------------------------------------------------------------------
    int           mblen(const char* s, size_t n);
    int           mbtowc(wchar_t* pwc, const char* s, size_t n);
    int           wctomb(char* s, wchar_t wc);
    size_t        mbstowcs(wchar_t* pwcs, const char* s, size_t n);
    size_t        wcstombs(char* s, const wchar_t* pwcs, size_t n);

#ifdef __cplusplus
};
#endif
