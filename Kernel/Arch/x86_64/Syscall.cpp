/*
 * Created by v1tr10l7 on 22.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>
#include <Arch/x86_64/CPU.hpp>

#include <Scheduler/Thread.hpp>
#include <System/InterruptManager.hpp>

namespace Syscall
{
    constexpr usize SYSCALL_VECTOR = 0x80;

    extern "C" void handleSyscall(ExecutionContext*);
    void            Initialize()
    {
        // TODO(v1tr10l7): fix the 0x80 syscall gate

        // auto handler
        //     = InterruptManager::AllocateHandler(0x80, nullptr, "cryptix");
        // handler->SetHandler(handleSyscall);
        //
        IDT::SetDPL(SYSCALL_VECTOR, DPL_RING3);
    }
    extern "C" void handleSyscall(ExecutionContext* ctx)
    {
        Arguments args{};

        args.Index            = ctx->RAX;
        args.Args[0]          = ctx->RDI;
        args.Args[1]          = ctx->RSI;
        args.Args[2]          = ctx->RDX;
        args.Args[3]          = ctx->R10;
        args.Args[4]          = ctx->R8;
        args.Args[5]          = ctx->R9;

        auto current          = CPU::GetCurrentThread();
        current->SavedContext = *ctx;

        Handle(args);
        ctx->RAX = args.ReturnValue;
    }
} // namespace Syscall
