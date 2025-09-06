/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

struct [[gnu::packed]] CPUContext
{
    u64 ds;
    u64 es;

    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 interruptVector;
    u64 errorCode;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

template <>
struct fmt::formatter<CPUContext> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const CPUContext& frame, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format(
                "ds: {:#x}, es: {:#x}\n"
                "rax: {:#x}, rbx: {:#x}, rcx: {:#x}, rdx: {:#x}\n"
                "rdi: {:#x}, rsi: {:#x}, rbp: {:#x}\n"
                "r8: {:#x}, r9: {:#x}, r10: {:#x}, r11: {:#x}\n"
                "r12: {:#x}, r13: {:#x}, r14: {:#x}, r15: {:#x}\n"
                "interruptVector: {:#x}, errorCode: {:#x}\n"
                "rip: {:#x}, cs: {:#x}, rflags: {:#x}, rsp: {:#x}, ss: {:#x}",
                frame.ds, frame.es, frame.rax, frame.rbx, frame.rcx, frame.rdx,
                frame.rdi, frame.rsi, frame.rbp, frame.r8, frame.r9, frame.r10,
                frame.r11, frame.r12, frame.r13, frame.r14, frame.r15,
                frame.interruptVector, frame.errorCode, frame.rip, frame.cs,
                frame.rflags, frame.rsp, frame.ss),
            ctx);
    }
};
