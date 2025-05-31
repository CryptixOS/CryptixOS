/*
 * Created by v1tr10l7 on 28.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Core/Types.hpp>

namespace GIC
{
    constexpr usize DISTRIBUTOR_BASE   = 0x8000000;
    constexpr usize REDISTRIBUTOR_BASE = 0x8010000;

#define GICD_BASE          0x8000000 // Distributor
#define GICR_BASE          0x8010000 // Redistributor

#define GICD_CTLR          (*(volatile uint32_t*)(GICD_BASE + 0x000))
#define GICD_ISENABLER(n)  (*(volatile uint32_t*)(GICD_BASE + 0x100 + (n) * 4))
#define GICD_ICENABLER(n)  (*(volatile uint32_t*)(GICD_BASE + 0x180 + (n) * 4))
#define GICD_IPRIORITYR(n) (*(volatile uint32_t*)(GICD_BASE + 0x400 + (n) * 4))

#define GICR_CTLR          (*(volatile uint32_t*)(GICR_BASE + 0x000))
#define GICR_ISENABLER0    (*(volatile uint32_t*)(GICR_BASE + 0x100))

    void Initialize(void)
    {
        // Enable GIC Distributor
        GICD_CTLR = 1; // Enable Distributor

        // Enable GIC Redistributor (for current CPU)
        GICR_CTLR = 1;

        // Enable CPU interface (System Register Interface)
        asm volatile(
            "msr ICC_IGRPEN1_EL1, %0\n" // Enable Group 1 interrupts
            "msr ICC_BPR1_EL1, %1\n"    // Set binary point (no preemption)
            "msr ICC_PMR_EL1, %2\n"     // Set priority mask to lowest priority
            "msr ICC_CTLR_EL1, %3\n"    // Enable EL1 interrupt control
            "isb"
            :
            : "r"(1), "r"(0), "r"(0xFF), "r"(1));
    }

#define GICC_BASE 0x8020000 // Example address from FDT
#define GICC_CTLR (*(volatile uint32_t*)(GICC_BASE + 0x000))
#define GICC_PMR  (*(volatile uint32_t*)(GICC_BASE + 0x004))

    void gic_v2_init(void)
    {
        // Enable GIC Distributor
        *reinterpret_cast<volatile u32*>(GICD_CTLR) = 1;

        // Enable CPU Interface
        GICC_PMR  = 0xFF; // Allow all priority levels
        GICC_CTLR = 1;    // Enable CPU Interface
    }

    void gic_enable_irq(int irq)
    {
        u64 reg              = irq / 32;
        u64 bit              = irq % 32;

        GICD_ISENABLER(reg)  = (1 << bit); // Enable the IRQ
        GICD_IPRIORITYR(irq) = 0x80;       // Set priority (medium)
    }
}; // namespace GIC
