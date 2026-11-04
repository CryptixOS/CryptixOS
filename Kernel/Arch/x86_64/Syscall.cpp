/*
 * Created by v1tr10l7 on 22.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Syscall.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.Arch.x86_64.Syscall;

import :API.Syscall;
import :Arch.x86_64.CPU;
import :Scheduler.Thread;
import :System.InterruptManager;

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
