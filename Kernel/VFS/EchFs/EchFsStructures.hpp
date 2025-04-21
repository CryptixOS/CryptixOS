/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Terminal.hpp>
#include <Prism/Core/Types.hpp>

#define ECHFS_DEBUG 1
#if ECHFS_DEBUG == 1
    #define EchFsDebug(label, format, value)                                   \
                                                                               \
        Log::Message("{}{}{}: {" format "}\n", AnsiColor::FOREGROUND_MAGENTA,  \
                     label, AnsiColor::FOREGROUND_WHITE, value);
#else
    #define EchFsDebug(...)
#endif

struct [[gnu::packed]] EchFsIdentityTable
{
    u8  JumpInstruction[4];
    u8  Signature[8];
    u64 TotalBlockCount;
    u64 MainDirectoryLength;
    u64 BytesPerBlock;
    u64 Reserved;
    u64 UUID[2];
};

constexpr usize ECHFS_END_OF_DIRECTORY  = 0x0000'0000'0000'0000;
constexpr usize ECHFS_DELETED_DIRECTORY = 0xffff'ffff'ffff'fffe;
constexpr usize ECHFS_ROOT_DIRECTORY_ID = 0xffff'ffff'ffff'ffff;

enum class EchFsDirectoryEntryType : u8
{
    eRegular   = 0,
    eDirectory = 1,
};

struct [[gnu::packed]] EchFsDirectoryEntry
{
    u64                     ParentID;
    EchFsDirectoryEntryType ObjectType;
    u8                      Name[201];
    u64                     UnixAtime;
    u64                     UnixMtime;
    u16                     Permissions;
    u16                     OwnerID;
    u16                     GroupID;
    u64                     UnixCtime;
    u64                     Payload;
    u64                     FileSize;
};
