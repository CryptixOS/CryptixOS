/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Firmware/ACPI/MADT.hpp>
#include <Library/Locking/SpinlockProtected.hpp>

#include <Prism/Containers/Bitmap.hpp>
#include <System/InterruptManager.hpp>

namespace InterruptManager
{
    namespace
    {
        Ref<InterruptController>                      s_IrqChip = nullptr;
        UnorderedMap<usize, Ref<InterruptDispatcher>> s_IrqMap;

        Spinlock                                      s_Lock;
        Bitmap                                        s_AllocatedIrqs;
    } // namespace

    IrqResult Handle(CPUContext* ctx)
    {
        IrqResult result = IrqResult::eNone;
        // s_IrqMap.With(
        //     [ctx, &result](auto& list)
        //     {
        //         auto it = list.Find(ctx->interruptVector);
        //         if (it == list.end()) return;
        //
        //         result = it->Value->Handle(ctx);
        //     });

#if CTOS_TARGET_X86_64
        auto it = s_IrqMap.Find(ctx->interruptVector);
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
