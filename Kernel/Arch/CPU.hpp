/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/CPU.hpp>
#elifdef CTOS_TARGET_AARCH64
    #include <Arch/aarch64/CPU.hpp>
#endif

#include <Memory/VMM.hpp>

#include <Prism/Memory/Memory.hpp>

#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/Time.hpp>

class ClockSource;
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
    CPU*         Current();
    u64          GetCurrentID();
    CPU*         GetCurrent();

    ClockSource* HighResolutionClock();

    Thread*      GetCurrentThread();

    void         PrepareThread(Thread* thread, Pointer pc, Pointer arg = 0);

    void         SaveThread(Thread* thread, CPUContext* ctx);
    void         LoadThread(Thread* thread, CPUContext* ctx);

    void         Reschedule(Timestep us);

    void         HaltAll();
    void         WakeUp(usize id, bool everyone);

    // NOTE(v1tr10l7): allows accessing usermode memory from ring0
    struct UserMemoryProtectionGuard
    {
        UserMemoryProtectionGuard();
        ~UserMemoryProtectionGuard();
    };

    template <typename F, typename... Args>
        requires(!SameAs<Prism::InvokeResultType<F, Args...>, void>)
    inline decltype(auto) AsUser(F&& f, Args&&... args)
    {
        UserMemoryProtectionGuard guard;

        return f(Forward<Args>(args)...);
    }
    template <typename T>
        requires(!SameAs<T, void>)
    inline decltype(auto) CopyFromUser(const T& value)
    {
        UserMemoryProtectionGuard guard;

        return value;
    }
    inline Path CopyStringFromUser(const char* string)
    {
        return AsUser([string]() -> Path { return string; });
    }

    template <typename T>
        requires(!SameAs<T, void>)
    inline void CopyToUser(T* userBuffer, const T& value)
    {
        UserMemoryProtectionGuard guard;

        *userBuffer = value;
    }
    inline void CopyStringToUser(StringView source, char* dest)
    {
        UserMemoryProtectionGuard guard;

        source.Copy(dest, source.Size());
    }

    template <typename F, typename... Args>
    inline static void AsUser(F&& f, Args&&... args)
    {
        UserMemoryProtectionGuard guard;
        f(Forward<Args>(args)...);
    }

    bool DuringSyscall();
    void OnSyscallEnter(usize index);
    void OnSyscallLeave();
}; // namespace CPU
