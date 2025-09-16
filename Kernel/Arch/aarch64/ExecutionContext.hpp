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
    u64 X0;
    u64 X1;
    u64 X2;
    u64 X3;
    u64 X4;
    u64 X5;
    u64 X6;
    u64 X7;
    u64 X8;
    u64 X9;
    u64 X10;
    u64 X11;
    u64 X12;
    u64 X13;
    u64 X14;
    u64 X15;
    u64 X16;
    u64 X17;
    u64 X18;
    u64 X19;
    u64 X20;
    u64 X21;
    u64 X22;
    u64 X23;
    u64 X24;
    u64 X25;
    u64 X26;
    u64 X27;
    u64 X28;
    CtAliasedField(u64, X29, FramePointer);
    CtAliasedField(u64, X30, Link);
    u64 Flags;
    CtAliasedField(u64, PC, ProgramCounter);
    CtAliasedField(u64, SP, StackPointer);
    u64 TLS;
    u64 ErrorCode;
};

template <>
struct fmt::formatter<ExecutionContext> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const ExecutionContext& frame, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format(
                "X0: {:#x}, X1: {:#x}, X2: {:#x}, X3: {:#x}\n"
                "X4: {:#x}, X5: {:#x}, X6: {:#x}, X7: {:#x}\n"
                "X8: {:#x}, X9: {:#x}, X10: {:#x}, X11: {:#x}\n"
                "X12: {:#x}, X13: {:#x}, X14: {:#x}, X15: {:#x}\n"
                "X16: {:#x}, X17: {:#x}, X18: {:#x}\n"
                "X19: {:#x}, X20: {:#x}, X21: {:#x}, X22: {:#x}\n"
                "X23: {:#x}, X24: {:#x}, X25: {:#x}, X26: {:#x}\n"
                "X27: {:#x}, X28: {:#x}, X29(fp): {:#x}, X30(lr): {:#x}\n"
                "flags: {:#x}, pc: {:#x}, sp: {:#x}, tls: {:#x}\n"
                "ErrorCode: {:#x}",
                frame.X0, frame.X1, frame.X2, frame.X3, frame.X4, frame.X5,
                frame.X6, frame.X7, frame.X8, frame.X9, frame.X10, frame.X11,
                frame.X12, frame.X13, frame.X14, frame.X15, frame.X16,
                frame.X17, frame.X18, frame.X19, frame.X20, frame.X21,
                frame.X22, frame.X23, frame.X24, frame.X25, frame.X26,
                frame.X27, frame.X28, frame.X29, frame.X30, frame.Flags,
                frame.PC, frame.SP, frame.TLS, frame.ErrorCode),
            ctx);
    }
};
