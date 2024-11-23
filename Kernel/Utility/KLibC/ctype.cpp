/*
 * Created by v1tr10l7 on 23.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ctype.h"

extern "C"
{
    int isalnum(int c) { return isalpha(c) || isdigit(c); }
    int isalpha(int c) { return isupper(c) || islower(c); }
    int isblank(int c) { return c == ' ' || c == '\t'; }
    int iscntrl(int c) { return (c >= 0x00 && c <= 0x1f) || (c == 0x7f); }
    int isdigit(int c) { return c >= '0' && c <= '9'; }
    int isgraph(int c) { return c >= '!' && c <= '~'; }
    int islower(int c) { return c >= 'a' && c <= 'z'; }
    int isprint(int c) { return isgraph(c) || c == ' '; }
    int ispunct(int c)
    {
        return (c >= '!' && c <= '/') || (c >= ':' && c <= '@')
            || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
    }
    int isspace(int c)
    {
        return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t'
            || c == '\v';
    }
    int isupper(int c) { return c >= 'A' && c <= 'Z'; }
    int isxdigit(int c) { return isdigit(c) || (c >= 'A' && c <= 'F'); }
    int tolower(int c) { return isupper(c) ? c + 0x20 : c; }
    int toupper(int c) { return islower(c) ? c - 0x20 : c; }
} // extern "C"
