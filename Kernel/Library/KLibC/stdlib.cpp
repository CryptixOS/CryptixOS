/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <stdlib.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Prism/Memory/Pointer.hpp>

module Cryptix:Impl.Library.KLibC.stdlib;

import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;
import :Memory.Allocator.KernelHeap;

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
