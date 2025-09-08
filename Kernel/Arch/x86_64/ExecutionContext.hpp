/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>

struct CTOS_PACKED ExecutionContext
{
    u64 DS;
    u64 ES;

    u64 RAX;
    u64 RBX;
    u64 RCX;
    u64 RDX;
    u64 RSI;
    u64 RDI;
    u64 RBP;
    u64 R8;
    u64 R9;
    u64 R10;
    u64 R11;
    u64 R12;
    u64 R13;
    u64 R14;
    u64 R15;

    u64 InterruptVector;
    u64 ErrorCode;

    CtAliasedField(u64, RIP, ProgramCounter);
    CtAliasedField(u64, CS, CodeSegmentSelector);
    CtAliasedField(u64, RFlags, Flags);
    CtAliasedField(u64, RSP, StackPointer);
    CtAliasedField(u64, SS, StackSegmentSelector);
};

template <>
struct fmt::formatter<ExecutionContext> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const ExecutionContext& frame, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format(
                "DS: {:#x}, ES: {:#x}\n"
                "RAX: {:#x}, RBX: {:#x}, RCX: {:#x}, RDX: {:#x}\n"
                "RDI: {:#x}, RSI: {:#x}, RBP: {:#x}\n"
                "R8: {:#x}, R9: {:#x}, R10: {:#x}, R11: {:#x}\n"
                "R12: {:#x}, R13: {:#x}, R14: {:#x}, R15: {:#x}\n"
                "InterruptVector: {:#x}, ErrorCode: {:#x}\n"
                "RIP: {:#x}, CS: {:#x}, RFlags: {:#x}, RSP: {:#x}, SS: {:#x}",
                frame.DS, frame.ES, frame.RAX, frame.RBX, frame.RCX, frame.RDX,
                frame.RDI, frame.RSI, frame.RBP, frame.R8, frame.R9, frame.R10,
                frame.R11, frame.R12, frame.R13, frame.R14, frame.R15,
                frame.InterruptVector, frame.ErrorCode, frame.RIP, frame.CS,
                frame.RFlags, frame.RSP, frame.SS),
            ctx);
    }
};
