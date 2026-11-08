/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

// Kernel Address Sanitizer: built with -fsanitize=kernel-address (the meson
// option 'kasan', or -DCTOS_KASAN=ON with CMake), every load and store calls
// into Debug/Kasan.cpp, which looks the address up in shadow memory, one byte
// for every 8 bytes of memory, and panics if it isn't addressable:
//  - free pages: the PMM poisons pages it frees and unpoisons the ones it
//    hands out
//  - the heap: past the requested size up to the slab object's end (out of
//    bounds) and freed objects (use after free)
//  - globals: the redzones the compiler puts after each one
// Shadow memory covers the direct map up to the top of RAM and the kernel
// image. Other addresses (MMIO and kernel virtual allocations) aren't
// checked, and stack instrumentation is off.
#ifndef CTOS_KASAN
    #define CTOS_KASAN 0
#endif

namespace Kasan
{
    inline constexpr bool ENABLED = CTOS_KASAN != 0;

    // Shadow byte values: 0 means all 8 bytes are addressable, 1-7 that only
    // that many leading bytes are, and the rest say why none are
    enum class ShadowType : u8
    {
        eUnpoisoned8Bytes = 0,
        eGlobalRedzone    = 0xf9,
        eHeapRedzone      = 0xfa,
        eHeapFree         = 0xfb,
        ePageFree         = 0xff,
    };

    // Maps the shadow memory and starts checking; after VMM::Initialize()
    void Initialize();
    bool IsInitialized();

    void PoisonImpl(Pointer address, usize size, ShadowType type);
    void UnpoisonImpl(Pointer address, usize size);
    usize AddressableSizeImpl(Pointer object, usize capacity);

    // Marks [address, address + size) as not addressable; both 8 byte aligned
    inline void Poison(Pointer address, usize size, ShadowType type)
    {
        if constexpr (ENABLED) PoisonImpl(address, size, type);
    }
    // Marks [address, address + size) as addressable; address 8 byte aligned
    inline void Unpoison(Pointer address, usize size)
    {
        if constexpr (ENABLED) UnpoisonImpl(address, size);
    }

    // Heap hooks. An object of `capacity` bytes (its slab size class, or
    // its pages) of which `size` were requested: the rest is a redzone.
    inline void OnAllocate(Pointer object, usize size, usize capacity)
    {
        if constexpr (!ENABLED) return;
        UnpoisonImpl(object, size);
        usize redzone = (size + 7) & ~7zu;
        if (capacity > redzone)
            PoisonImpl(object.Offset(redzone), capacity - redzone,
                       ShadowType::eHeapRedzone);
    }
    // The first 8 bytes stay addressable: the slab allocator keeps its free
    // list in freed objects
    inline void OnFree(Pointer object, usize capacity)
    {
        if constexpr (!ENABLED) return;
        // The whole first granule, even after a smaller allocation left it
        // partially addressable
        UnpoisonImpl(object, capacity < 8 ? capacity : 8);
        if (capacity > 8)
            PoisonImpl(object.Offset(8), (capacity - 8) & ~7zu,
                       ShadowType::eHeapFree);
    }
    // How many leading bytes of an object are addressable, which is the
    // size it was allocated with; `capacity` without KASAN
    inline usize AddressableSize(Pointer object, usize capacity)
    {
        if constexpr (!ENABLED) return capacity;
        return AddressableSizeImpl(object, capacity);
    }
}; // namespace Kasan
