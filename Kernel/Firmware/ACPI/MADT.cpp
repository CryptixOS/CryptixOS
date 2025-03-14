/*
 * Created by vitriol1744 on 17.11.2024.
 * Copyright (c) 2022-2023, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Firmware/ACPI/MADT.hpp>

namespace MADT
{
    using Prism::Pointer;
    namespace
    {
        struct MADT
        {
            ACPI::SDTHeader Header;
            u32             LapicAddress;
            u32             Flags;
            u8              Entries[];
        } __attribute__((packed));

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

        static MADT*                s_MADT;

        std::vector<LapicEntry*>    s_LapicEntries;
        std::vector<IoApicEntry*>   s_IoApicEntries;
        std::vector<IsoEntry*>      s_IsoEntries;
        std::vector<LapicNmiEntry*> s_LapicNmiEntries;
    } // namespace

    void Initialize()
    {
        s_MADT = ACPI::GetTable<MADT>(MADT_SIGNATURE);
        Assert(s_MADT);

        for (usize off = 0; s_MADT->Header.Length - sizeof(MADT) - off >= 2;)

        {
            Header*   header    = Pointer(s_MADT->Entries + off).As<Header>();

            EntryType entryType = static_cast<EntryType>(header->ID);
            LogTrace("MADT: Found '{}' entry",
                     magic_enum::enum_name(entryType));

            switch (static_cast<EntryType>(header->ID))
            {
                case EntryType::eProcessorLapic:
                    s_LapicEntries.push_back(
                        reinterpret_cast<LapicEntry*>(header));
                    break;
                case EntryType::eIoApic:
                    s_IoApicEntries.push_back(
                        reinterpret_cast<IoApicEntry*>(header));
                    break;
                case EntryType::eInterruptSourceOverride:
                    s_IsoEntries.push_back(reinterpret_cast<IsoEntry*>(header));
                    break;
                case EntryType::eNmiSource: break;
                case EntryType::eLapicNmi:
                    s_LapicNmiEntries.push_back(
                        reinterpret_cast<LapicNmiEntry*>(header));
                    break;
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

            off += std::max(header->Length, u8(2));
        }

        LogInfo("MADT: Initialized");
    }

    bool                         LegacyPIC() { return s_MADT->Flags & 0x01; }

    std::vector<LapicEntry*>&    GetLapicEntries() { return s_LapicEntries; }
    std::vector<IoApicEntry*>&   GetIoApicEntries() { return s_IoApicEntries; }
    std::vector<IsoEntry*>&      GetIsoEntries() { return s_IsoEntries; }
    std::vector<LapicNmiEntry*>& GetLapicNmiEntries()
    {
        return s_LapicNmiEntries;
    }
} // namespace MADT
