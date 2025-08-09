/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

namespace ACPI
{
    consteval u64 MakeSignature(const char str[4])
    {
        return (static_cast<u64>(str[0]) << 0) | (static_cast<u64>(str[1]) << 8)
             | (static_cast<u64>(str[2]) << 16)
             | (static_cast<u64>(str[3]) << 24);
    }

    enum class TableID
    {
        eRSDP = MakeSignature("RSDP"),
        eRSDT = MakeSignature("RSDT"),
        eXSDT = MakeSignature("XSDT"),
        eFADT = MakeSignature("FACP"),
        eFACS = MakeSignature("FACS"),
        eDSDT = MakeSignature("DSDT"),
        eSSDT = MakeSignature("SSDT"),
        eMADT = MakeSignature("APIC"),
        eHPET = MakeSignature("HPET"),
        eBGRT = MakeSignature("BGRT"),
        eECDT = MakeSignature("ECDT"),
        eERST = MakeSignature("ERST"),
        eFPDT = MakeSignature("FPDT"),
        eGTDT = MakeSignature("GTDT"),
        eHEST = MakeSignature("HEST"),
        eSRAT = MakeSignature("SRAT"),
        eMCFG = MakeSignature("MCFG"),
    };
}; // namespace ACPI
