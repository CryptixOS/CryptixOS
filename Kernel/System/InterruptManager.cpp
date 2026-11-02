/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
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
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Prism/String/StringUtils.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.System.InterruptManager;

import :Arch.CPU;
import :Firmware.ACPI.MADT;
import :Library.Locking.SpinlockProtected;
import :System.InterruptManager;

namespace InterruptManager
{
    namespace
    {
        Ref<InterruptController>                      s_IrqChip = nullptr;
        UnorderedMap<usize, Ref<InterruptDispatcher>> s_IrqMap;

        Spinlock                                      s_Lock;
        Bitmap                                        s_AllocatedIrqs;
    } // namespace

    IrqResult Handle(ExecutionContext* ctx)
    {
        IrqResult result = IrqResult::eNone;
        // s_IrqMap.With(
        //     [ctx, &result](auto& list)
        //     {
        //         auto it = list.Find(ctx->InterruptVector);
        //         if (it == list.end()) return;
        //
        //         result = it->Value->Handle(ctx);
        //     });

#if CTOS_TARGET_X86_64
        auto it = s_IrqMap.Find(ctx->InterruptVector);
        if (it == s_IrqMap.end()) return result;

        result = it->Value->Handle(ctx);
#endif
        return result;
    }

    Ref<InterruptController> Controller() { return s_IrqChip; }
    void                     SetController(Ref<InterruptController> ctrl)
    {
        Assert(ctrl);
        s_IrqChip                          = ctrl;

        CTOS_UNUSED static auto initialize = [&]() -> bool
        {
            s_AllocatedIrqs.Allocate(255);
            return true;
        }();
    }

    Ref<InterruptDispatcher> AllocateHandler(u32 hint, Device* device,
                                             StringView moduleName)
    {
        if (hint < 0x20) hint += 0x20;

        ScopedLock      guard(s_Lock);
        Optional<usize> allocatedIRQ = NullOpt;
        if (MADT::LegacyPIC())
        {
            if ((hint >= 0x20 && hint <= (0x20 + 15)) && !s_AllocatedIrqs[hint])
                allocatedIRQ = hint;
        }

        for (usize i = hint; !allocatedIRQ && i < 256; i++)
            if (!s_AllocatedIrqs[i]) allocatedIRQ = i;
        if (!allocatedIRQ) return nullptr;

        auto handler = CreateRef<InterruptDispatcher>(
            device, s_IrqChip.Raw(), static_cast<i64>(*allocatedIRQ - 0x20));
        s_IrqMap[*allocatedIRQ] = handler;
        return handler;
    }

    void Mask(u32 irq) { s_IrqChip->Mask(irq); }
    void Unmask(u32 irq) { s_IrqChip->Unmask(irq); }

    void SendEOI(u32 irq) { s_IrqChip->SendEOI(irq); }
}; // namespace InterruptManager
