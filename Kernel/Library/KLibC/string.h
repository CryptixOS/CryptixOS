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
#endif
    void*  memcpy(void* dest, const void* src, size_t len) throw();
    int    memcmp(const void* ptr1, const void* ptr2, size_t len) throw();
    void*  memset(void* dest, int ch, size_t len) throw();
    void*  memmove(void* dest, const void* src, size_t len) throw();
    void*  memchr(const void* ptr, int ch, size_t len);

    size_t strlen(const char* str) throw();
    char*  strdup(const char* str) throw();

    char*  strchr(const char* str, int ch);

    int    strcmp(const char* str1, const char* str2) throw();
    int    strncmp(const char* str1, const char* str2, size_t len) throw();

    char*  strcpy(char* dest, const char* src) throw();
    char*  strncpy(char* dest, const char* src, size_t len) throw();

    char*  strstr(const char* str, const char* substr);
    void   strrev(char* str);

#ifdef __cplusplus
} // extern "C"
#endif
