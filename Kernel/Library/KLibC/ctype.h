/*
 * Created by v1tr10l7 on 23.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    int isalnum(int c);
    int isalpha(int c);
    int islower(int c);
    int isupper(int c);
    int isdigit(int c);
    int isxdigit(int c);
    int iscntrl(int c);
    int isgraph(int c);
    int isspace(int c);
    int isblank(int c);
    int isprint(int c);
    int ispunct(int c);
    int toupper(int c);
    int tolower(int c);

#ifdef __cplusplus
} // extern "C"
#endif
