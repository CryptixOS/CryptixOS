/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace KernelHeap
{
    void    Initialize();

    // TODO(v1tr10l7): alignment
    Pointer Allocate(usize bytes);
    Pointer Callocate(usize bytes);
    Pointer Reallocate(Pointer address, usize size);

    void    Free(Pointer memory);
} // namespace KernelHeap
