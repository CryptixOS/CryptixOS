#include <cstddef>
#include <cstdint>

#define SyscallInvoker "syscall"

namespace mlibc
{
#pragma region
    inline constexpr size_t SYS_WRITE = 0;

#pragma endregion

    static inline uintptr_t SyscallAsm0(uintptr_t n)
    {
        uintptr_t ret;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm1(uintptr_t n, uintptr_t a1)
    {
        uintptr_t ret;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm2(uintptr_t n, uintptr_t a1, uintptr_t a2)
    {
        uintptr_t ret;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm3(uintptr_t n, uintptr_t a1, uintptr_t a2,
                                        uintptr_t a3)
    {
        uintptr_t ret;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm4(uintptr_t n, uintptr_t a1, uintptr_t a2,
                                        uintptr_t a3, uintptr_t a4)
    {
        uintptr_t          ret;
        register uintptr_t r10 asm("r10") = a4;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm5(uintptr_t n, uintptr_t a1, uintptr_t a2,
                                        uintptr_t a3, uintptr_t a4,
                                        uintptr_t a5)
    {
        uintptr_t          ret;
        register uintptr_t r10 asm("r10") = a4;
        register uintptr_t r8 asm("r8")   = a5;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
                     : "rcx", "r11", "memory");
        return ret;
    }

    static inline uintptr_t SyscallAsm6(uintptr_t n, uintptr_t a1, uintptr_t a2,
                                        uintptr_t a3, uintptr_t a4,
                                        uintptr_t a5, uintptr_t a6)
    {
        uintptr_t          ret;
        register uintptr_t r10 asm("r10") = a4;
        register uintptr_t r8 asm("r8")   = a5;
        register uintptr_t r9 asm("r9")   = a6;
        asm volatile(SyscallInvoker
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
                       "r"(r9)
                     : "rcx", "r11", "memory");
        return ret;
    }

#define GetMacro(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define InvokeSyscall(...)                                                     \
    GetMacro(__VA_ARGS__, SyscallAsm6, SyscallAsm5, SyscallAsm4, SyscallAsm3,  \
             SyscallAsm2, SyscallAsm1, SyscallAsm0)(__VA_ARGS__)

    inline uintptr_t DoSyscall(uintptr_t n, uintptr_t a1, uintptr_t a2,
                               uintptr_t a3, uintptr_t a4, uintptr_t a5,
                               uintptr_t a6)
    {
        return InvokeSyscall(n, a1, a2, a3, a4, a5, a6);
    }
    inline uintptr_t DoSyscall(uintptr_t n, uintptr_t a1, uintptr_t a2,
                               uintptr_t a3, uintptr_t a4, uintptr_t a5)
    {
        return InvokeSyscall(n, a1, a2, a3, a4, a5);
    }
    inline uintptr_t DoSyscall(uintptr_t n, uintptr_t a1, uintptr_t a2,
                               uintptr_t a4)
    {
        return InvokeSyscall(n, a1, a2, a4);
    }
    inline uintptr_t DoSyscall(uintptr_t n, uintptr_t a1, uintptr_t a2)
    {
        return InvokeSyscall(n, a1, a2);
    }
    inline uintptr_t DoSyscall(uintptr_t n, uintptr_t a1)
    {
        return InvokeSyscall(n, a1);
    }
    inline uintptr_t DoSyscall(uintptr_t n) { return InvokeSyscall(n); }

    template <typename... Args>
    inline uintptr_t Syscall(Args&&... args)
    {
        return DoSyscall(uintptr_t(args)...);
    }
} // namespace mlibc
