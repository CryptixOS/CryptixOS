/*
 * Created by v1tr10l7 on 22.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "CPU.hpp"

#include "API/Syscall.hpp"
#include "Scheduler/Thread.hpp"

namespace Syscall
{
    constexpr usize SYSCALL_VECTOR = 0x80;

    extern "C" void handleSyscall(CPUContext*);
    void            Initialize()
    {
        auto handler = IDT::GetHandler(SYSCALL_VECTOR);
        handler->SetHandler(handleSyscall);
        handler->Reserve();

        IDT::SetDPL(SYSCALL_VECTOR, DPL_RING3);
    }
    extern "C" void handleSyscall(CPUContext* ctx)
    {
        Arguments args{};

        args.index            = ctx->rax;
        args.args[0]          = ctx->rdi;
        args.args[1]          = ctx->rsi;
        args.args[2]          = ctx->rdx;
        args.args[3]          = ctx->r10;
        args.args[4]          = ctx->r8;
        args.args[5]          = ctx->r9;

        Thread* current       = CPU::GetCurrentThread();
        current->SavedContext = *ctx;

        Handle(args);
        ctx->rax = args.returnValue;
    }
} // namespace Syscall
