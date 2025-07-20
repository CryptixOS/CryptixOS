/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Firmware/ACPI/MADT.hpp>

#include <Prism/String/StringUtils.hpp>

namespace MADT
{
    namespace
    {
        struct [[gnu::packed]] MADT
        {
            ACPI::SDTHeader Header;
            u32             LapicAddress;
            u32             Flags;
            union
            {
                u8            Entries[];
                LapicEntry    LapicEntries[];
                IoApicEntry   IoApicEntries[];
                IsoEntry      IsoEntries[];
                LapicNmiEntry LapicNmiEntries[];
            };
        };

        constexpr const char* MADT_SIGNATURE = "APIC";

        enum class EntryType
        {
            eProcessorLapic          = 0x00,
            eIoApic                  = 0x01,
            eInterruptSourceOverride = 0x02,
            eNmiSource               = 0x03,
            eLapicNmi                = 0x04,
            eLapicAddressOverride    = 0x05,

            eProcessorLocalX2Apic    = 0x09,
            eLocalX2ApicNmi          = 0x0a,
            eGicCpuInterface         = 0x0b,
            eGicDistributor          = 0x0c,
            eGicMsiFrame             = 0x0d,
        };

        static MADT*          s_MADT;

        Vector<LapicEntry>    s_LapicEntries;
        Vector<IoApicEntry>   s_IoApicEntries;
        Vector<IsoEntry>      s_IsoEntries;
        Vector<LapicNmiEntry> s_LapicNmiEntries;
    } // namespace

    void Initialize()
    {
        s_MADT = ACPI::GetTable<MADT>(MADT_SIGNATURE);
        Assert(s_MADT);

        for (usize off = 0; s_MADT->Header.Length - sizeof(MADT) - off >= 2;)

        {
            Pointer   entry     = &s_MADT->Entries[off];
            Header*   header    = entry.ToHigherHalf<Header*>();

            EntryType entryType = static_cast<EntryType>(header->ID);
            LogTrace("MADT: Found '{}' entry",
                     ToString(entryType).Substr(1));

            switch (static_cast<EntryType>(header->ID))
            {
                case EntryType::eProcessorLapic:
                {
                    auto& lapicEntry = s_LapicEntries.EmplaceBack();
                    Memory::Copy(&lapicEntry, header, sizeof(LapicEntry));
                    break;
                }
                case EntryType::eIoApic:
                {
                    auto& ioapicEntry = s_IoApicEntries.EmplaceBack();
                    Memory::Copy(&ioapicEntry, header, sizeof(IoApicEntry));
                    break;
                }
                case EntryType::eInterruptSourceOverride:
                {
                    auto& isoEntry = s_IsoEntries.EmplaceBack();
                    Memory::Copy(&isoEntry, header, sizeof(IsoEntry));
                    break;
                }
                case EntryType::eNmiSource: break;
                case EntryType::eLapicNmi:
                {
                    auto& lapicNmiEntry = s_LapicNmiEntries.EmplaceBack();
                    Memory::Copy(&lapicNmiEntry, header, sizeof(LapicNmiEntry));
                    break;
                }
                case EntryType::eLapicAddressOverride: break;
                case EntryType::eProcessorLocalX2Apic: break;
                case EntryType::eGicCpuInterface: break;
                case EntryType::eGicDistributor: break;
                case EntryType::eGicMsiFrame: break;

                default:
                    LogWarn("MADT: Encountered unrecognized header(id = {})",
                            header->ID);
                    break;
            }

            off += Max(header->Length, u8(2));
        }

        LogInfo("MADT: Initialized");
    }

    bool                   LegacyPIC() { return s_MADT->Flags & 0x01; }

    Vector<LapicEntry>&    GetLapicEntries() { return s_LapicEntries; }
    Vector<IoApicEntry>&   GetIoApicEntries() { return s_IoApicEntries; }
    Vector<IsoEntry>&      GetIsoEntries() { return s_IsoEntries; }
    Vector<LapicNmiEntry>& GetLapicNmiEntries() { return s_LapicNmiEntries; }
} // namespace MADT
