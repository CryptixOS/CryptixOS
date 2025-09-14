/*
 * Created by v1tr10l7 on 11.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/InterruptController.hpp>
#include <Firmware/DeviceTree/DeviceTree.hpp>
#include <Prism/Memory/Ref.hpp>

class GIC : public InterruptController
{
  public:
    GIC(DeviceTree::Node& node);

    static bool           Register();

    virtual ErrorOr<void> Initialize() override;
    virtual ErrorOr<void> Shutdown() override;

    virtual ErrorOr<void> Mask(u32 irq) override;
    virtual ErrorOr<void> Unmask(u32 irq) override;

    virtual ErrorOr<void> SendEOI(u32 irq) override;

  private:
    static DeviceTree::Driver s_Driver;

    // 4.1.2 Distributor register map
    struct DistributorRegisters
    {
        static constexpr usize IRQ_CONTROL_ENABLE = Bit(0);
        static constexpr usize IRQ_COUNT_OFFSET   = 0;
        static constexpr u32   IRQ_COUNT_MASK     = Bit(5) - 1;

        // GICD_CTLR
        u32                    Control;
        // GICR_TYPER
        u32                    Type;
        // GICD_IIDR
        u32                    ID;
        u32                    Reserved0[5];
        u32                    Reserved1[8];
        u32                    reserved2[16];
        // GICD_IGROUPn
        u32                    InterruptGroup[32];
        // GICD_ISENABLERn
        u32                    InterruptSetEnable[32];
        // GICD_ICENABLERn
        u32                    InterruptClearEnable[32];
        // GICD_ISPENDRn
        u32                    InterruptSetPending[32];
        // GICD_ICPENDRn
        u32                    InterruptClearPending[32];
        // GICD_ISACTIVERn
        u32                    InterruptSetActive[32];
        // GICD_ICACTIVERn
        u32                    InterruptClearActive[32];
        // GICD_IPRIORITYRn
        u32                    InterruptPriority[255];
        u32                    Reserved3;
        // GICD_ITARGETSRn
        u32                    InterruptProcessorTargets[255];
        u32                    Reserved4;
        // GICD_ICFGRn
        u32                    InterruptConfiguration[64];
        u32                    Reserved5[64];
        // GICD_NSACRn
        u32                    NonSecureAccessControl[64];
        // GICD_SGIR
        u32                    SGI;
        u32                    Reserved6[3];
        // GICD_CPENDSGIRn
        u32                    SgiClearPending[4];
        // GICD_SPENDSGIRn
        u32                    SgiSetPending[4];
        u32                    Reserved7[40];
        u32                    Reserved8[12];
    };
    static_assert(sizeof(DistributorRegisters) == 0x1000);
    struct CPU_InterfaceRegisters
    {
        static constexpr usize IRQ_CONTROL_ENABLE = Bit(0);
        static constexpr usize ID_VERSION_OFFSET  = 16;
        static constexpr u32   ID_VERSION_MASK    = Bit(4) - 1;

        // GICC_CTLR
        u32                    Control;
        // GICC_PMR, only the 8 bottom bits are valid
        u32                    InterruptPriorityMask;
        // GICC_BPR
        u32                    BinaryPoint;
        // GICC_IAR
        u32                    InterruptAcknowledge;
        // GICC_EOIR
        u32                    EndOfInterrupt;
        // GICC_RPR, only the 8 bottom bits are valid
        u32                    RunningPriority;
        // GICC_HPPIR
        u32                    HighestPriorityPendingInterrupt;
        // GICC_ABPR
        u32                    AliasedBinaryPoint;
        // GICC_AIAR
        u32                    AliasedInterruptAcknowledge;
        // GICC_AEOIR
        u32                    AliasedEndOfInterrupt;
        // GICC_AHPPIR
        u32                    AliasedHighestPriorityPendingInterrupt;
        u32                    Reserved0[5];
        u32                    ImplementationDefined0[36];
        // GICC_APRn
        u32                    ActivePriorities[4];
        // GICC_NSAPRn
        u32                    NonSecureActivePriorities[4];
        u32                    Reserved1[3];
        // GICC_IIDR
        u32                    ID;
        u32                    Reserved2[960];
        // GICC_DIR
        u32                    DeactivateInterrupt;
    };
    static_assert(sizeof(CPU_InterfaceRegisters) == 0x1004);

    DeviceTree::Register             m_DistributorRegister   = {0, 0};
    volatile DistributorRegisters*   m_Distributor           = nullptr;

    DeviceTree::Register             m_RedistributorRegister = {0, 0};
    volatile CPU_InterfaceRegisters* m_CPUInterface          = nullptr;

    static ErrorOr<void>             Probe(DeviceTree::Node& node);
};
