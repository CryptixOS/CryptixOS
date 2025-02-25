/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdlib.h>

#include "Common.hpp"

#include "Memory/KernelHeap.hpp"

extern "C"
{
    void* calloc(size_t num, size_t size) throw()
    {
        return KernelHeap::Callocate(num * size);
    }

    void  free(void* ptr) throw() { KernelHeap::Free(ptr); }

    void* malloc(size_t size) throw() { return KernelHeap::Allocate(size); }

    void* realloc(void* oldptr, size_t size) throw()
    {
        return KernelHeap::Reallocate(oldptr, size);
    }

    long long strtoll(const char* str, char** str_end, int base) throw()
    {
        return 0;
    }

    void abort() throw() { Panic("Abort"); }
} // extern "C"
