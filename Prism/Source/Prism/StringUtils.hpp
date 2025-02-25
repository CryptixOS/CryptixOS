/*
 * Created by v1tr10l7 on 21.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Math.hpp>
#include <Prism/Types.hpp>

namespace Prism
{
    template <typename T>
    T ToNumber(const char* str, usize length)
    {
        T     integer      = 0;
        bool  isNegative   = str[0] == '-';

        usize index        = isNegative;
        usize stringLength = length, power = stringLength - isNegative;

        for (; index < stringLength; index++)
            integer += (str[index] - 48) * Math::PowerOf<T>(10, --power);

        return (isNegative) ? -integer : integer;
    }
    template <typename T>
    T ToNumber(const char* str)
    {
        return ToNumber<T>(str, std::strlen(str));
    }

    template <typename T>
    char* ToString(T value, char* str, i32 base)
    {
        T    i          = 0;
        bool isNegative = false;

        if (value == 0)
        {
            str[i++] = '0';
            str[i]   = 0;
            return str;
        }

        if (value < 0 && base == 10)
        {
            isNegative = true;
            value      = -value;
        }

        while (value != '\0')
        {
            T rem    = value % base;
            str[i++] = (rem > 9) ? static_cast<char>((rem - 10) + 'a')
                                 : static_cast<char>(rem + '0');
            value    = value / base;
        }

        if (isNegative) str[i++] = '-';
        str[i]  = '\0';

        T start = 0;
        T end   = i - 1;
        while (start < end)
        {
            char c         = *(str + start);
            *(str + start) = *(str + end);
            *(str + end)   = c;
            start++;
            end--;
        }

        return str;
    }
}; // namespace Prism
