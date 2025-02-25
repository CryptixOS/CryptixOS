/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Common.hpp"

void asan_verify(uintptr_t at, usize size, uintptr_t ip, bool rw, bool abort)
{
    return;
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_register_globals()
{
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_unregister_globals()
{
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_report_store_n_noabort()
{
}
extern "C" __attribute__((no_sanitize("address"))) void
__asan_report_load_n_noabort()
{
}

#define ASAN_REPORT_LOAD(size)                                                 \
    extern "C" __attribute__((no_sanitize(                                     \
        "address"))) void __asan_report_load##size##_noabort(uintptr_t addr)   \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    false, false);                                             \
    }
#define ASAN_REPORT_STORE(size)                                                \
    extern "C" __attribute__((no_sanitize(                                     \
        "address"))) void __asan_report_store##size##_noabort(uintptr_t addr)  \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    false, false);                                             \
    }
#define ASAN_LOAD_NOABORT(size)                                                \
    extern "C" __attribute__((no_sanitize(                                     \
        "address"))) void __asan_load##size##_noabort(uintptr_t addr)          \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    false, false);                                             \
    }
#define ASAN_LOAD_ABORT(size)                                                  \
    extern "C" __attribute__((no_sanitize("address"))) void __asan_load##size( \
        uintptr_t addr)                                                        \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    false, true);                                              \
    }
#define ASAN_STORE_NOABORT(size)                                               \
    extern "C" __attribute__((no_sanitize(                                     \
        "address"))) void __asan_store##size##_noabort(uintptr_t addr)         \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    true, false);                                              \
    }
#define ASAN_STORE_ABORT(size)                                                 \
    extern "C" __attribute__((                                                 \
        no_sanitize("address"))) void __asan_store##size(uintptr_t addr)       \
    {                                                                          \
        asan_verify(addr, size,                                                \
                    (uintptr_t)__builtin_extract_return_addr(                  \
                        __builtin_return_address(0)),                          \
                    true, true);                                               \
    }

ASAN_REPORT_LOAD(1)
ASAN_REPORT_LOAD(2)
ASAN_REPORT_LOAD(4)
ASAN_REPORT_LOAD(8)
ASAN_REPORT_LOAD(16)

ASAN_REPORT_STORE(1)
ASAN_REPORT_STORE(2)
ASAN_REPORT_STORE(4)
ASAN_REPORT_STORE(8)
ASAN_REPORT_STORE(16)

ASAN_LOAD_ABORT(1)
ASAN_LOAD_ABORT(2)
ASAN_LOAD_ABORT(4)
ASAN_LOAD_ABORT(8)
ASAN_LOAD_ABORT(16)
ASAN_LOAD_NOABORT(1)
ASAN_LOAD_NOABORT(2)
ASAN_LOAD_NOABORT(4)
ASAN_LOAD_NOABORT(8)
ASAN_LOAD_NOABORT(16)

ASAN_STORE_ABORT(1)
ASAN_STORE_ABORT(2)
ASAN_STORE_ABORT(4)
ASAN_STORE_ABORT(8)
ASAN_STORE_ABORT(16)
ASAN_STORE_NOABORT(1)
ASAN_STORE_NOABORT(2)
ASAN_STORE_NOABORT(4)
ASAN_STORE_NOABORT(8)
ASAN_STORE_NOABORT(16)

extern "C" __attribute__((no_sanitize("address"))) void
__asan_load_n(uintptr_t addr, usize size)
{
    asan_verify(
        addr, size,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
        false, true);
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_store_n(uintptr_t addr, usize size)
{
    asan_verify(
        addr, size,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
        true, true);
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_loadN_noabort(uintptr_t addr, usize size)
{
    asan_verify(
        addr, size,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
        false, false);
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_storeN_noabort(uintptr_t addr, usize size)
{
    asan_verify(
        addr, size,
        (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)),
        true, false);
}

extern "C" __attribute__((no_sanitize("address"))) void
__asan_after_dynamic_init(uintptr_t addr, usize size)
{
    return;
}
extern "C" __attribute__((no_sanitize("address"))) void
__asan_before_dynamic_init(uintptr_t addr, usize size)
{
    return;
}
extern "C" __attribute__((no_sanitize("address"))) void
__asan_handle_no_return(uintptr_t addr, usize size)
{
    return;
}
