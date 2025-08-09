/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Arch/CPU.hpp>

#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Utility/PathView.hpp>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>

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
    using ToFormattablePtr = Conditional<
        IsPointerV<T>,
        Conditional<IsConvertibleV<StringView, T>, NullableString, const void*>,
        T>;

    template <typename... Ts>
    auto Ptr(const std::tuple<Ts...>& tup)
    {
        return std::tuple<ToFormattablePtr<Ts>...>(tup);
    }

    template <typename T>
    constexpr auto ConvertArgument(upointer value)
    {
        if constexpr (IsSameV<RemoveCvRefType<RemoveReferenceType<T>>,
                              Prism::PathView>)
            return CPU::AsUser(
                [value]() -> PathView
                {
                    return PathView(
                        StringView(reinterpret_cast<const char*>(value)));
                });
        else if constexpr (IsPointerV<T>) return reinterpret_cast<T>(value);
        else return static_cast<T>(value);
    }

    class WrapperBase
    {
      public:
        virtual ~WrapperBase()                                       = default;
        virtual ErrorOr<upointer> Run(Array<upointer, 6> args) const = 0;
    };
    template <typename Func>
    class Wrapper : public WrapperBase
    {
      public:
        using Sign = Signature<RemoveCvRefType<Func>>;

        constexpr Wrapper(const char* name, Func* func)
            : Function(func)
            , Name(name)
        {
        }

        ErrorOr<upointer> Run(Array<upointer, 6> arr) const
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
        u64      Index;
        u64      Args[6];

        upointer ReturnValue;

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
        ePRead64          = 17,
        ePWrite64         = 18,
        eAccess           = 21,
        ePipe             = 22,
        eSchedYield       = 24,
        eDup              = 32,
        eDup2             = 33,
        eNanoSleep        = 35,
        ePid              = 39,
        eFork             = 57,
        eExecve           = 59,
        eExit             = 60,
        eWait4            = 61,
        eKill             = 62,
        eUname            = 63,
        eFCntl            = 72,
        eTruncate         = 76,
        eFTruncate        = 77,
        eGetCwd           = 79,
        eChDir            = 80,
        eFChDir           = 81,
        eRename           = 82,
        eMkDir            = 83,
        eRmDir            = 84,
        eCreat            = 85,
        eLink             = 86,
        eUnlink           = 87,
        eSymlink          = 88,
        eReadLink         = 89,
        eChMod            = 90,
        eFChMod           = 91,
        eChown            = 92,
        eFChown           = 93,
        eLChown           = 94,
        eUmask            = 95,
        eGetTimeOfDay     = 96,
        eGetResourceLimit = 97,
        eGetResourceUsage = 98,
        eGetUid           = 102,
        eGetGid           = 104,
        eSetUid           = 105,
        eSetGid           = 106,
        eGet_eUid         = 107,
        eGet_eGid         = 108,
        eSet_pGid         = 109,
        eGet_pPid         = 110,
        eGetPgrp          = 111,
        eSetSid           = 112,
        eSetReUid         = 113,
        eSetReGid         = 114,
        eSetResUid        = 117,
        eSetResGid        = 119,
        eGet_pGid         = 121,
        eSid              = 124,
        eUTime            = 132,
        eStatFs           = 137,
        eArchPrCtl        = 158,
        eSync             = 162,
        eSetTimeOfDay     = 164,
        eMount            = 165,
        eUMount           = 166,
        eReboot           = 169,
        eInitModule       = 175,
        eGetTid           = 186,
        eGetDents64       = 217,
        eClockGetTime     = 228,
        eClockNanoSleep   = 230,
        ePanic            = 255,
        eOpenAt           = 257,
        eMkDirAt          = 258,
        eMkNodAt          = 259,
        eFStatAt          = 262,
        eUnlinkAt         = 263,
        eRenameAt         = 264,
        eLinkAt           = 265,
        eSymlinkAt        = 266,
        eReadLinkAt       = 267,
        eFChModAt         = 268,
        ePSelect6         = 270,
        eUtimensAt        = 280,
        eDup3             = 292,
        eSyncFs           = 306,
        eFinitModule      = 313,
        eRenameAt2        = 316,
    };

    StringView GetName(usize index);
    void       RegisterHandler(usize                                        index,
                               std::function<ErrorOr<upointer>(Arguments&)> handler,
                               String                                       name);
#define RegisterSyscall(id, handler)                                           \
    s_Syscalls[id] = new Wrapper<decltype(handler)>(#handler, handler);

    void InstallAll();
    void Handle(Arguments& args);
}; // namespace Syscall
