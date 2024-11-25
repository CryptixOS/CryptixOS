/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "MADT.hpp"

namespace MADT
{
    namespace
    {
        struct MADT
        {
            SDTHeader header;
            u32       lapicAddress;
            u32       flags;
            u8        entries[];
        };

        inline constexpr const char* MADT_SIGNATURE                    = "APIC";
        inline constexpr const u8    ENTRY_TYPE_PROCESSOR_LAPIC        = 0;
        inline constexpr const u8    ENTRY_TYPE_IO_APIC                = 1;
        inline constexpr const u8    ENTRY_TYPE_IO_APIC_ISO            = 2;
        inline constexpr const u8    ENTRY_TYPE_IO_APIC_NMI_SOURCE     = 3;
        inline constexpr const u8    ENTRY_TYPE_LAPIC_NMI              = 4;
        inline constexpr const u8    ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE = 5;
        inline constexpr const u8    ENTRY_TYPE_PROCESSOR_LOCAL_x2APIC = 6;

        static MADT*                 madt;

        std::vector<LAPICEntry*>     lapicEntries;
        std::vector<IOAPICEntry*>    ioapicEntries;
        std::vector<ISOEntry*>       isoEntries;
        std::vector<LAPIC_NMIEntry*> lapicNMIEntries;
    } // namespace

    void Initialize()
    {
        madt = ACPI::GetTable<MADT>(MADT_SIGNATURE);
        Assert(madt != nullptr);

        for (usize off = 0; madt->header.length - sizeof(MADT) - off >= 2;)

        {
            Header* header = reinterpret_cast<Header*>(madt->entries + off);

            switch (header->id)
            {
                case ENTRY_TYPE_PROCESSOR_LAPIC:
                    LogTrace("MADT: Found local APIC #{}", lapicEntries.size());
                    lapicEntries.push_back(
                        reinterpret_cast<LAPICEntry*>(header));
                    break;
                case ENTRY_TYPE_IO_APIC:
                    LogTrace("MADT: Found IO APIC #{}", ioapicEntries.size());
                    ioapicEntries.push_back(
                        reinterpret_cast<IOAPICEntry*>(header));
                    break;
                case ENTRY_TYPE_IO_APIC_ISO:
                    LogTrace("MADT: Found ISO #{}", isoEntries.size());
                    isoEntries.push_back(reinterpret_cast<ISOEntry*>(header));
                    break;
                case ENTRY_TYPE_IO_APIC_NMI_SOURCE: break;
                case ENTRY_TYPE_LAPIC_NMI:
                    LogTrace("MADT: Found NMI #{}", lapicNMIEntries.size());
                    lapicNMIEntries.push_back(
                        reinterpret_cast<LAPIC_NMIEntry*>(header));
                    break;
                case ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE: break;
                case ENTRY_TYPE_PROCESSOR_LOCAL_x2APIC: break;

                default:
                    LogWarn("MADT: Encountered unrecognized header(id = {})",
                            header->id);
                    break;
            }

            off += std::max(header->length, u8(2));
        }

        LogTrace("MADT: Initialized");
    }

    bool                          LegacyPIC() { return madt->flags & 0x01; }

    std::vector<LAPICEntry*>&     GetLAPICEntries() { return lapicEntries; }
    std::vector<IOAPICEntry*>&    GetIOAPICEntries() { return ioapicEntries; }
    std::vector<ISOEntry*>&       GetISOEntries() { return isoEntries; }
    std::vector<LAPIC_NMIEntry*>& GetLAPIC_NMIEntries()
    {
        return lapicNMIEntries;
    }
} // namespace MADT
