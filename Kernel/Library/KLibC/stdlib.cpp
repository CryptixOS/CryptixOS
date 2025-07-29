/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdlib.h>

#include <Common.hpp>
#include <Memory/Allocator/KernelHeap.hpp>

extern "C"
{
    void  free(void* ptr) throw() { KernelHeap::Free(ptr); }
    void* malloc(size_t size) throw() { return KernelHeap::Allocate(size); }
    void* realloc(void* oldptr, size_t size) throw()
    {
        return KernelHeap::Reallocate(oldptr, size);
    }

    void abort() throw() { panic("Abort"); }
} // extern "C"
