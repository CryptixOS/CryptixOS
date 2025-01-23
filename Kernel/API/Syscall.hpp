/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>

namespace Syscall
{
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
        eRead         = 0,
        eWrite        = 1,
        eOpen         = 2,
        eClose        = 3,
        eStat         = 4,
        eFStat        = 5,
        eLStat        = 6,
        ePoll         = 7,
        eLSeek        = 8,
        eMMap         = 9,
        eMUnMap       = 11,
        eSigProcMask  = 14,
        eIoCtl        = 16,
        eAccess       = 21,
        eDup          = 32,
        eDup2         = 33,
        eGetPid       = 39,
        eFork         = 57,
        eExecve       = 59,
        eExit         = 60,
        eWait4        = 61,
        eKill         = 62,
        eUname        = 63,
        eFcntl        = 72,
        eTruncate     = 76,
        eFTruncate    = 77,
        eGetCwd       = 79,
        eChDir        = 80,
        eFChDir       = 81,
        eMkDir        = 83,
        eRmDir        = 84,
        eGetTimeOfDay = 96,
        eGetUid       = 102,
        eGetGid       = 104,
        eGet_eUid     = 107,
        eGet_eGid     = 108,
        eSet_pGid     = 109,
        eGet_pPid     = 110,
        eGetPgrp      = 111,
        eSetSid       = 112,
        eGet_pGid     = 121,
        eGetSid       = 124,
        eUTime        = 132,
        eArchPrCtl    = 158,
        eSetTimeOfDay = 164,
        eReboot       = 169,
        eGetTid       = 186,
        eGetDents64   = 217,
        eClockGetTime = 228,
        eNanoSleep    = 230,
        ePanic        = 255,
        eOpenAt       = 257,
        eMkDirAt      = 258,
        eFStatAt      = 262,
        eDup3         = 292,
    };

    void RegisterHandler(usize                                         index,
                         std::function<ErrorOr<uintptr_t>(Arguments&)> handler,
                         std::string                                   name);
#define RegisterSyscall(index, handler)                                        \
    ::Syscall::RegisterHandler(std::to_underlying(index), handler, #handler)

    void InstallAll();
    void Handle(Arguments& args);
}; // namespace Syscall
