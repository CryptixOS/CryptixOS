/*
 * Created by v1tr10l7 on 22.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>
#include <Arch/x86_64/CPU.hpp>

#include <Scheduler/Thread.hpp>

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

        args.Index            = ctx->rax;
        args.Args[0]          = ctx->rdi;
        args.Args[1]          = ctx->rsi;
        args.Args[2]          = ctx->rdx;
        args.Args[3]          = ctx->r10;
        args.Args[4]          = ctx->r8;
        args.Args[5]          = ctx->r9;

        auto current       = CPU::GetCurrentThread();
        current->SavedContext = *ctx;

        Handle(args);
        ctx->rax = args.ReturnValue;
    }
} // namespace Syscall
