/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/Arch.hpp>
#include <Arch/aarch64/Drivers/GIC.hpp>
#include <System/InterruptManager.hpp>

namespace Arch
{
    /* === Timer system-register helpers === */
    /* CNTP_TVAL_EL0 is written with a 32-bit signed value (relative ticks) */
    static inline void CoreTimerSet(u64 ticks)
    {
        asm volatile("msr cntp_cval_el0, %0" ::"r"(ticks));
    }
    static inline void CoreTimerEnable()
    {
        asm volatile("msr cntp_ctl_el0, %0" ::"r"(1ull)
                     : "memory"); // ENABLE=1, IMASK=0
    }

    CTOS_UNUSED static inline void CoreTimerDisable()
    {
        asm volatile("msr cntp_ctl_el0, %0" ::"r"(0ull) : "memory");
    }
    static inline u64 CoreTimerReadCounter()
    {
        u64 val;
        asm volatile("mrs %0, cntpct_el0" : "=r"(val));
        return val;
    }

    static inline u64 CoreTimerReadFreq()
    {
        u64 val;
        asm volatile("mrs %0, cntfrq_el0" : "=r"(val));
        return val;
    }

/* === Timer and tick logic === */
#define TIMER_IRQ_ID 30u /* EL1 physical timer PPI (SBSA recommended) */

    static u64  s_TicksPerMs;            /* ticks in one millisecond */
    static u64  s_TickIntervalMs = 1000; /* default 1 second tick */

    /* schedule next expiration in ms */
    static void SchedulerNextTickMs(u64 ms)
    {
        u64 freq  = CoreTimerReadFreq();
        u64 now   = CoreTimerReadCounter();
        u64 ticks = now + ((freq * ms) / 1000ul);

        CoreTimerSet((u64)ticks);
    }

    /* === IRQ handler (called from your platform IRQ vector at EL1) ===
     * This is the function you should call from your IRQ entry/dispatch code.
     */
    void TimerIrqHandler(::Ref<GIC> irqChip)
    {
        u64 id = irqChip->Acknowledge();

        if (id == TIMER_IRQ_ID)
        {
            /* IMPORTANT: program next timer before EOI, so ISTATUS clears */
            SchedulerNextTickMs(s_TickIntervalMs);

            /* do minimal work: print a dot (avoid heavy work in IRQ) */
            LogTrace(".");

            /* tell GIC we're done with this interrupt */
            irqChip->SendEOI(id);
        }
        else
            /* other interrupts: forward to your IRQ dispatcher or simply EOI */
            if (id < 1020)
                /* spurious usually 1023/1022 depending on GIC version */
                /* handle or route other IRQs here */
                irqChip->SendEOI(id);
    }

    void EnableInterrupts();
    /* === Kernel entry (EL1) ===
     * Do minimal init: enable CPU interrupts, GIC CPU interface, enable PPI 30,
     * arm the timer, and enable interrupts in PSTATE.
     *
     * You must ensure this function runs at EL1 with interrupts disabled
     * initially.
     */
    void FireTimerInterrupt(::Ref<GIC> irqChip)
    {
        LogTrace("kernel: timer demo starting\n");

        /* enable the EL1 physical timer interrupt (PPI 30) in the GIC for this
         * CPU */
        irqChip->Unmask(TIMER_IRQ_ID);
        LogTrace("timer interrupt unmasked");

        /* compute s_TicksPerMs for info/debug if you want (not strictly needed)
         */
        {
            u64 freq     = CoreTimerReadFreq();
            s_TicksPerMs = (u64)(freq / 1000ULL);
        }

        /* schedule first tick and enable the timer */
        SchedulerNextTickMs(10);
        CoreTimerEnable();

        /* enable IRQs in PSTATE: clear DAIF.I (assembly) */
        asm volatile("msr daifclr, #2" ::: "memory"); /* clear I bit (irq) */

        LogTrace("kernel: timer armed, now idling\n");
        EnableInterrupts();

        /* simple idle loop; timer IRQ will print dots */
        while (1) /* wfi to save host CPU cycles */
            asm volatile("wfi");
    }

    KERNEL_INIT_SECTION
    void Initialize()
    {
        auto irqChip = GIC::Register();
        Assert(irqChip);
        InterruptManager::SetController(irqChip);
        irqChip->Initialize();
        FireTimerInterrupt(irqChip);
    }

    CTOS_NORETURN void Halt()
    {
        for (;;) __asm__ volatile("msr daifclr, #0b1111; wfi");
    }
    void   Pause() { __asm__ volatile("isb" ::: "memory"); }

    time_t GetEpoch() { return 0; }

    void   EnableInterrupts() { __asm__ volatile("msr daifclr, #0b1111"); }
    void   DisableInterrupts() { __asm__ volatile("msr daifset, #0b1111"); }
    bool   ExchangeInterruptFlag(bool enabled)
    {
        u64 interruptsDisabled;
        __asm__ volatile("mrs %0, daif" : "=r"(interruptsDisabled));

        if (enabled) EnableInterrupts();
        else DisableInterrupts();
        return !interruptsDisabled;
    }

    void Reboot() {}
} // namespace Arch
