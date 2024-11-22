/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdlib.h>

extern "C"
{
    // TODO(v1tr10l7): memory allocation
    void*     calloc(size_t num, size_t size) throw() { return nullptr; }

    void      free(void* ptr) throw() {}

    void*     malloc(size_t size) throw() { return nullptr; }

    void*     realloc(void* oldptr, size_t size) throw() { return nullptr; }

    long long strtoll(const char* str, char** str_end, int base) throw()
    {
        return 0;
    }

    void abort() throw() { __builtin_unreachable(); }
} // extern "C"
