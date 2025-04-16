/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/VMM.hpp>
#include <Time/TimeStep.hpp>

#if CTOS_ARCH == CTOS_ARCH_X86_64
    #include <Arch/x86_64/CPU.hpp>
#elif CTOS_ARCH == CTOS_ARCH_AARCH64
    #include <Arch/aarch64/CPU.hpp>
#endif

struct Thread;
struct CPUContext;
namespace CPU
{
    constexpr usize KERNEL_STACK_SIZE = 64_kib;
    constexpr usize USER_STACK_SIZE   = 2_mib;

    void            DumpRegisters(CPUContext* ctx);
    bool            GetInterruptFlag();
    void            SetInterruptFlag(bool enabled);
    bool            SwapInterruptFlag(bool enabled);

    u64             GetOnlineCPUsCount();

    struct CPU;
    CPU*    GetCurrent();
    u64     GetCurrentID();
    Thread* GetCurrentThread();

    void    PrepareThread(Thread* thread, uintptr_t pc, uintptr_t arg = 0);

    void    SaveThread(Thread* thread, CPUContext* ctx);
    void    LoadThread(Thread* thread, CPUContext* ctx);

    void    Reschedule(TimeStep us);

    void    HaltAll();
    void    WakeUp(usize id, bool everyone);

    // NOTE(v1tr10l7): allows accessing usermode memory from ring0
    struct UserMemoryProtectionGuard
    {
        UserMemoryProtectionGuard();
        ~UserMemoryProtectionGuard();
    };

    template <typename F, typename... Args>
        requires(!std::same_as<std::invoke_result_t<F, Args...>, void>)
    inline decltype(auto) AsUser(F&& f, Args&&... args)
    {
        UserMemoryProtectionGuard guard;

        return f(std::forward<Args>(args)...);
    }
    template <typename F, typename... Args>
    inline static void AsUser(F&& f, Args&&... args)
    {
        UserMemoryProtectionGuard guard;
        f(std::forward<Args>(args)...);
    }
}; // namespace CPU
