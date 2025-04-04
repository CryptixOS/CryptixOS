/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <string.h>

#include <stdlib.h>

#include "Prism/Core/Types.hpp"

extern "C"
{
#ifndef CTOS_TARGET_X86_64
    void* memcpy(void* dest, const void* src, usize len) throw()
    {
        u8*       d = static_cast<u8*>(dest);
        const u8* s = static_cast<const u8*>(src);

        for (usize i = 0; i < len; i++) d[i] = s[i];

        return dest;
    }

    void* memset(void* dest, int ch, usize len) throw()
    {
        u8* d = static_cast<u8*>(dest);

        for (usize i = 0; i < len; i++) d[i] = static_cast<u8>(ch);

        return dest;
    }

    void* memmove(void* dest, const void* src, usize len) throw()
    {
        u8*       d = static_cast<u8*>(dest);
        const u8* s = static_cast<const u8*>(src);

        if (src > dest)
            for (usize i = 0; i < len; i++) d[i] = s[i];
        else if (src < dest)
            for (usize i = len; i > 0; i--) d[i - 1] = s[i - 1];

        return dest;
    }

    int memcmp(const void* ptr1, const void* ptr2, usize len) throw()
    {
        const u8* p1 = static_cast<const u8*>(ptr1);
        const u8* p2 = static_cast<const u8*>(ptr2);

        for (usize i = 0; i < len; i++)
            if (p1[i] != p2[i]) return p1[i] < p2[i] ? -1 : 1;

        return 0;
    }
#endif

    void* memchr(const void* ptr, int ch, usize len)
    {
        const u8* src = static_cast<const u8*>(ptr);

        while (len-- > 0)
        {
            if (*src == ch) return const_cast<u8*>(src);
            src++;
        }

        return nullptr;
    }

    usize strlen(const char* str) throw()
    {
        usize length = 0;
        while (str[length]) length++;
        return length;
    }

    char* strdup(const char* str) throw()
    {
        usize len        = strlen(str) + 1;

        void* new_string = malloc(len);
        if (!new_string) return nullptr;

        return static_cast<char*>(memcpy(new_string, str, len));
    }

    char* strcat(char* dest, const char* src) throw()
    {
        char* ptr = dest + strlen(dest);
        while (*src != '\0') *ptr++ = *src++;

        *ptr = '\0';
        return dest;
    }

    char* strncat(char* dest, const char* src, usize len) throw()
    {
        char* ptr = dest + strlen(dest);
        while (*src != '\0' && len-- > 0) *ptr++ = *src++;

        *ptr = '\0';
        return dest;
    }

    /*char* strchr(const char* str, int ch) throw()
    {
        while (*str && *str != ch) str++;
        return const_cast<char*>(ch == *str ? str : nullptr);
    }*/

    int strcmp(const char* str1, const char* str2) throw()
    {
        while (*str1 && *str2 && *str1 == *str2)
        {
            str1++;
            str2++;
        }
        return *str1 - *str2;
    }

    int strncmp(const char* str1, const char* str2, usize len) throw()
    {
        while (*str1 && *str2 && *str1 == *str2 && len-- > 0)
        {
            str1++;
            str2++;
        }

        return len != 0 ? *str1 - *str2 : 0;
    }

    char* strcpy(char* dest, const char* src) throw()
    {
        char* ptr = dest;
        while (*src != '\0')
        {
            *dest = *src;
            dest++;
            src++;
        }

        *dest = '\0';
        return ptr;
    }

    char* strncpy(char* dest, const char* src, usize len) throw()
    {
        char* ret = dest;
        while (*src != '\0' && len-- > 0)
        {
            *dest = *src;
            dest++;
            src++;
        }

        *dest = '\0';
        return ret;
    }

    /*char* strstr(const char* str, const char* substr) throw()
    {
        const char *a = str, *b = substr;
        while (true)
        {
            if (*b == 0) return const_cast<char*>(str);
            if (*a == 0) return nullptr;

            if (*a++ != *b++)
            {
                a = ++str;
                b = substr;
            }
        }
    }*/

    void strrev(char* str)
    {
        char  a;
        usize len = strlen(reinterpret_cast<const char*>(str));

        for (usize i = 0, j = len - 1; i < j; i++, j--)
        {
            a      = str[i];
            str[i] = str[j];
            str[j] = a;
        }
    }
} // extern "C"
