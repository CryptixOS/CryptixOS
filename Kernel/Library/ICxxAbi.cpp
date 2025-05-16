/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <cxxabi.h>
#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include <immintrin.h>
#endif

using DestructorFunction = void (*)(void*);

struct AtExitFunctionEntry
{
    DestructorFunction func;
    void*              objptr;
};

constexpr const u64 ATEXIT_MAX_FUNCS = 128;

extern "C"
{
    AtExitFunctionEntry __atexit_funcs[ATEXIT_MAX_FUNCS];
    u64                 __atexit_func_count = 0;

    void*               __dso_handle        = 0;

    int __cxa_atexit(DestructorFunction func, void* objptr, void* dso)
    {
        if (__atexit_func_count >= ATEXIT_MAX_FUNCS) return -1;
        __atexit_funcs[__atexit_func_count].func   = func;
        __atexit_funcs[__atexit_func_count].objptr = objptr;
        __atexit_func_count++;

        return 0;
    };

    void __cxa_finalize(void* f)
    {
        u64 i = __atexit_func_count;
        if (!f)
        {
            while (i--)
            {
                if (__atexit_funcs[i].func)
                    (*__atexit_funcs[i].func)(__atexit_funcs[i].objptr);
            };
            return;
        };

        i64 findex = -1;
        for (u64 j = 0; j < __atexit_func_count; j++)
        {
            if (__atexit_funcs[j].func == f)
            {
                (*__atexit_funcs[j].func)(__atexit_funcs[j].objptr);
                __atexit_funcs[j].func = 0;
                findex                 = j;
            }
        }
        if (findex < 0) return;
        for (u64 j = findex; j < __atexit_func_count; j++)
        {
            __atexit_funcs[j].func   = __atexit_funcs[j + 1].func;
            __atexit_funcs[j].objptr = __atexit_funcs[j + 1].objptr;
        }
        --__atexit_func_count;
    }

    void __cxa_pure_virtual()
    {
        Panic("__cxa_pure_virtual()");
        __builtin_unreachable();
    }

    uintptr_t __stack_chk_guard = 0;

    [[noreturn]]
    void __stack_chk_fail()
    {
        Panic("icxxabi: stack smashing detected!");
        __builtin_unreachable();
    }

    [[maybe_unused]]
    void __attribute__((no_stack_protector)) __guard_setup(void)
    {
        unsigned char* p;
        if (__stack_chk_guard != 0) return;
#if CTOS_ARCH == CTOS_ARCH_X86_64
        if (_rdrand64_step(
                reinterpret_cast<unsigned long long*>(&__stack_chk_guard))
            && __stack_chk_guard != 0)
            return;
#endif
        /* If a random generator can't be used, the protector switches the guard
           to the "terminator canary".  */
        p = reinterpret_cast<unsigned char*>(&__stack_chk_guard);
        p[sizeof(__stack_chk_guard) - 1] = 255;
        p[sizeof(__stack_chk_guard) - 2] = '\n';
        p[0]                             = 0;
    }

    namespace __cxxabiv1
    {
        int __cxa_guard_acquire(__guard* guard)
        {
            if ((*guard) & 0x0001) return 0;
            if ((*guard) & 0x0100) __cxa_guard_abort(guard);

            *guard |= 0x0100;
            return 1;
        }

        void __cxa_guard_release(__guard* guard) { *guard |= 0x0001; }

        void __cxa_guard_abort(__guard* guard)
        {
            Panic("__cxa_guard_abort({})", static_cast<void*>(guard));
            __builtin_unreachable();
        }
    } // namespace __cxxabiv1
}

namespace icxxabi
{
    using ConstructorFunction = void (*)();

    extern "C" ConstructorFunction __init_array_start[];
    extern "C" ConstructorFunction __init_array_end[];

    void                           Initialize()
    {
        EarlyLogTrace("icxxabi: Calling global constructors...");
        for (ConstructorFunction* entry = __init_array_start;
             entry < __init_array_end; entry++)
        {
            ConstructorFunction constructor = *entry;
            constructor();
        }

        LogInfo("icxxabi: Global constructors invoked");
    }
} // namespace icxxabi
