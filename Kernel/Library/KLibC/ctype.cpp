/*
 * Created by v1tr10l7 on 23.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/String/StringUtils.hpp>

using namespace StringUtils;

extern "C"
{
    int isalnum(int c) { return IsAlphanumeric(c); }
    int isalpha(int c) { return IsAlpha(c); }
    int isdigit(int c) { return IsDigit(c); }
    int toupper(int c) { return ToUpper(c); }
} // extern "C"
