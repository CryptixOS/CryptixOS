/*
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
// The replaceable global allocation functions have to belong to the global module, so they
// cannot live in a unit of `Cryptix` (names declared there are attached to the named module).
// They are defined here, in an ordinary translation unit, and forward to the kernel heap.
#include <Prism/Core/Types.hpp>

import Cryptix;

void* operator new(usize size) { return KernelHeap::Callocate(size); }
void* operator new(usize size, AlignmentType)
{
    return KernelHeap::Callocate(size);
}
void* operator new[](usize size) { return KernelHeap::Callocate(size); }
void* operator new[](usize size, AlignmentType)
{
    return KernelHeap::Callocate(size);
}
void operator delete(void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete(void* memory, AlignmentType) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete(void* memory, usize) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory) noexcept { KernelHeap::Free(memory); }
void operator delete[](void* memory, AlignmentType) noexcept
{
    KernelHeap::Free(memory);
}
void operator delete[](void* memory, usize) noexcept
{
    KernelHeap::Free(memory);
}
