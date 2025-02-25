/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
    #define restrict
#endif

#define _IOFBF       0
#define _IOLBF       1
#define _IONBF       2
#define FOPEN_MAX    1024
#define FILENAME_MAX 1024
#define EOF          (-1)

    typedef size_t FILE;
    typedef size_t fpos_t;

    //    extern FILE*   stdin;
    extern FILE*   stdout;
    extern FILE*   stderr;
    size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
    int    fputws(const wchar_t* str, FILE* stream);

    //    int            remove(const char* filename);
    //    int            rename(const char* old, const char* _new);
    //    FILE*          tmpfile(void);
    //    char*          tmpnam(char* s);
    //    int            fclose(FILE* stream);
    int    fflush(FILE* stream);
    //    FILE* fopen(const char* restrict filename, const char* restrict
    //    GetMode); FILE* freopen(const char* restrict filename, const char*
    //    restrict GetMode,
    //                  FILE* restrict stream);
    //    void  setbuf(FILE* restrict stream, char* restrict buf);
    //    int   setvbuf(FILE* restrict stream, char* restrict buf, int GetMode,
    //                  size_t size);
    int    fprintf(FILE* restrict stream, const char* restrict format, ...)
        __attribute__((format(printf, 2, 3)));
    //    int fscanf(FILE* restrict stream, const char* restrict format, ...);
    int printf(const char* restrict format, ...)
        __attribute__((format(printf, 1, 2)));
    //    int scanf(const char* restrict format, ...);
    int snprintf(char* restrict s, size_t n, const char* format, ...)
        __attribute__((format(printf, 3, 4)));
    int sprintf(char* restrict s, const char* format, ...)
        __attribute__((format(printf, 2, 3)));
    //    int sscanf(const char* restrict s, const char* restrict format, ...);
    //    int vfprintf(FILE* restrict stream, const char* restrict format,
    //                 va_list args) __attribute__((format(printf, 2, 0)));
    //    int vfscanf(FILE* restrict stream, const char* restrict format,
    //                va_list arg);
    int vprintf(const char* restrict format, va_list args)
        __attribute((format(printf, 1, 0)));
    //    int vscanf(const char* restrict format, va_list arg);
    int vsnprintf(char* restrict s, size_t n, const char* restrict format,
                  va_list args) __attribute__((format(printf, 3, 0)));
    int vsprintf(char* restrict s, const char* format, va_list args)
        __attribute__((format(printf, 2, 0)));
    //    int   vsscanf(const char* restrict s, const char* restrict format,
    //                  va_list arg);
    //    int   fgetc(FILE* stream);
    //    char* fgets(char* restrict s, int n, FILE* restrict stream);
    int fputc(int c, FILE* stream);
    int fputs(const char* restrict s, FILE* restrict stream);
    //    int   getc(FILE* stream);
    //    int   getchar(void);
    //    char* gets(char* s);
    //    int   putc(int c, FILE* stream);
    int putchar(int c);
//    int   puts(const char* s);
//    int   ungetc(int c, FILE* stream);
#ifdef __cplusplus
} // extern "C"
#endif
