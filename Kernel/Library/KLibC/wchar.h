/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <stddef.h>

#ifndef __WINT_TYPE__
    #define __WINT_TYPE__ unsigned int
#endif

typedef __WINT_TYPE__ wint_t;

#ifdef __cplusplus
extern "C"
{
#endif

    wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t count);
    wchar_t* wmemmove(wchar_t* dest, const wchar_t* src, size_t count);
    wchar_t* wmemset(wchar_t* dest, wchar_t ch, size_t count);
    wchar_t* wmemchr(const wchar_t* ptr, wchar_t ch, size_t count);

    int      wmemcmp(const wchar_t* lhs, const wchar_t* rhs, size_t count);
    size_t   wcslen(const wchar_t* start);

#ifdef __cplusplus
} // extern "C"
#endif
