/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>
#include <Prism/Core/Types.hpp>

struct CTOS_PACKED CPUContext
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
    u64 X29; // FP
    u64 X30; // LR

    u64 Sp;
    u64 Pc;
    u64 Pstate;
};

template <>
struct fmt::formatter<CPUContext> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const CPUContext& frame, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format(
                "x0: {:#x}, x1: {:#x}, x2: {:#x}, x3: {:#x}\n"
                "x4: {:#x}, x5: {:#x}, x6: {:#x}, x7: {:#x}\n"
                "x8: {:#x}, x9: {:#x}, x10: {:#x}, x11: {:#x}\n"
                "x12: {:#x}, x13: {:#x}, x14: {:#x}, x15: {:#x}\n"
                "x16: {:#x}, x17: {:#x}, x18: {:#x}\n"
                "x19: {:#x}, x20: {:#x}, x21: {:#x}, x22: {:#x}\n"
                "x23: {:#x}, x24: {:#x}, x25: {:#x}, x26: {:#x}\n"
                "x27: {:#x}, x28: {:#x}, x29(fp): {:#x}, x30(lr): {:#x}\n"
                "sp: {:#x}, pc: {:#x}, pstate: {:#x}",
                frame.X0, frame.X1, frame.X2, frame.X3, frame.X4, frame.X5,
                frame.X6, frame.X7, frame.X8, frame.X9, frame.X10, frame.X11,
                frame.X12, frame.X13, frame.X14, frame.X15, frame.X16,
                frame.X17, frame.X18, frame.X19, frame.X20, frame.X21,
                frame.X22, frame.X23, frame.X24, frame.X25, frame.X26,
                frame.X27, frame.X28, frame.X29, frame.X30, frame.Sp, frame.Pc,
                frame.Pstate),
            ctx);
    }
};
