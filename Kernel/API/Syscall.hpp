/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Arch/CPU.hpp>

#include <Prism/Path.hpp>
#include <Prism/PathView.hpp>
#include <Prism/TypeTraits.hpp>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>

#include <type_traits>

namespace Syscall
{
    template <typename>
    struct Signature;

    template <typename Ret, typename... Args>
    struct Signature<Ret(Args...)>
    {
        using Type          = Ret(Args...);
        using ArgsContainer = std::tuple<Args...>;
        using RetType       = Ret;
    };

    struct NullableString
    {
        const char* String;
        explicit constexpr NullableString(const char* s)
            : String(s)
        {
        }
    };
    inline constexpr auto FormatAs(NullableString n)
    {
        return n.String ?: "(null)";
    }

    template <typename T>
    using ToFormattablePtr
        = Conditional<std::is_pointer_v<T>,
                      Conditional<std::is_constructible_v<StringView, T>,
                                  NullableString, const void*>,
                      T>;

    template <typename... Ts>
    auto Ptr(const std::tuple<Ts...>& tup)
    {
        return std::tuple<ToFormattablePtr<Ts>...>(tup);
    }

    template <typename T>
    constexpr auto ConvertArgument(uintptr_t value)
    {
        if constexpr (IsSameV<std::remove_cvref_t<std::remove_reference_t<T>>,
                              Prism::PathView>)
            return CPU::AsUser(
                [value]() -> PathView
                {
                    return PathView(
                        StringView(reinterpret_cast<const char*>(value)));
                });
        else if constexpr (std::is_pointer_v<T>)
            return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }

    class WrapperBase
    {
      public:
        virtual ~WrapperBase() = default;
        virtual ErrorOr<uintptr_t> Run(std::array<uintptr_t, 6> args) const = 0;
    };
    template <typename Func>
    class Wrapper : public WrapperBase
    {
      public:
        using Sign = Signature<std::remove_cvref_t<Func>>;

        constexpr Wrapper(const char* name, Func* func)
            : Function(func)
            , Name(name)
        {
        }

        ErrorOr<uintptr_t> Run(std::array<uintptr_t, 6> arr) const
        {
            typename Sign::ArgsContainer args;
            usize                        i = 0;

            // Convert array to actual function arguments

            {
                CPU::UserMemoryProtectionGuard guard;
                std::apply(
                    [&](auto&&... args)
                    {
                        (std::invoke([&]<typename T>(T& arg)
                                     { arg = ConvertArgument<T>(arr[i++]); },
                                     args),
                         ...);
                    },
                    args);
            }
            errno    = no_error;
            auto ret = std::apply(Function, args);
            if (!ret) return Error(ret.error());

            return ret;
        }

      private:
        Func*       Function;
        const char* Name;
    };

    struct Arguments
    {
        u64       Index;
        u64       Args[6];

        uintptr_t ReturnValue;

        template <typename T>
        inline T Get(usize index) const
        {
            return T(Args[index]);
        }
    };

    enum class ID : u64
    {
        eRead             = 0,
        eWrite            = 1,
        eOpen             = 2,
        eClose            = 3,
        eStat             = 4,
        eFStat            = 5,
        eLStat            = 6,
        ePoll             = 7,
        eLSeek            = 8,
        eMMap             = 9,
        eMProtect         = 10,
        eMUnMap           = 11,
        eSigProcMask      = 14,
        eIoCtl            = 16,
        eAccess           = 21,
        ePipe             = 22,
        eSchedYield       = 24,
        eDup              = 32,
        eDup2             = 33,
        eNanoSleep        = 35,
        eGetPid           = 39,
        eFork             = 57,
        eExecve           = 59,
        eExit             = 60,
        eWait4            = 61,
        eKill             = 62,
        eUname            = 63,
        eFcntl            = 72,
        eTruncate         = 76,
        eFTruncate        = 77,
        eGetCwd           = 79,
        eChDir            = 80,
        eFChDir           = 81,
        eMkDir            = 83,
        eRmDir            = 84,
        eCreat            = 85,
        eReadLink         = 89,
        eChMod            = 90,
        eUmask            = 95,
        eGetTimeOfDay     = 96,
        eGetResourceLimit = 97,
        eGetResourceUsage = 98,
        eGetUid           = 102,
        eGetGid           = 104,
        eGet_eUid         = 107,
        eGet_eGid         = 108,
        eSet_pGid         = 109,
        eGet_pPid         = 110,
        eGetPgrp          = 111,
        eSetSid           = 112,
        eGet_pGid         = 121,
        eGetSid           = 124,
        eUTime            = 132,
        eArchPrCtl        = 158,
        eSetTimeOfDay     = 164,
        eMount            = 165,
        eUMount           = 166,
        eReboot           = 169,
        eGetTid           = 186,
        eGetDents64       = 217,
        eClockGetTime     = 228,
        eClockNanoSleep   = 230,
        ePanic            = 255,
        eOpenAt           = 257,
        eMkDirAt          = 258,
        eFStatAt          = 262,
        eFChModAt         = 268,
        ePSelect6         = 270,
        eDup3             = 292,
    };

    StringView GetName(usize index);
    void       RegisterHandler(usize                                         index,
                               std::function<ErrorOr<uintptr_t>(Arguments&)> handler,
                               std::string                                   name);
#define RegisterSyscall(index, handler)                                        \
    ::Syscall::RegisterHandler(std::to_underlying(index), handler, #handler)
#define RegisterSyscall2(id, handler)                                          \
    s_Syscalls[id] = new Wrapper<decltype(handler)>(#handler, handler);

    void InstallAll();
    void Handle(Arguments& args);
}; // namespace Syscall
