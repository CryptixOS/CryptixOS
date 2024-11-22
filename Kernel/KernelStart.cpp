/*
 * Created by v1tr10l7 on 16.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <icxxabi>
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "Utility/Types.hpp"

// stubs
void* operator new(size_t size) { return (void*)1; }
void* operator new(usize size, std::align_val_t) { return (void*)1; }
void* operator new[](size_t size) { return (void*)1; }
void* operator new[](size_t size, std::align_val_t) { return (void*)1; }
void  operator delete(void* p) noexcept {}
void  operator delete(void* p, std::align_val_t) noexcept {}
void  operator delete(void* p, usize) noexcept {}
void  operator delete[](void* p) noexcept {}
void  operator delete[](void* p, std::align_val_t) noexcept {}

extern "C" void kernelStart()
{
    for (;;)
        __asm__ volatile("outb %b0, %w1" : : "a"('H'), "Nd"(0xe9) : "memory");
}
