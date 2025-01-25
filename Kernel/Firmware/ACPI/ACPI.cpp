/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/ACPI/MADT.hpp>

#include <Boot/BootInfo.hpp>
#include <Memory/VMM.hpp>

#include <uacpi/event.h>
#include <uacpi/uacpi.h>

namespace ACPI
{
    namespace
    {
        struct RSDP
        {
            char Signature[8];
            u8   Checksum;
            char OemID[6];
            u8   Revision;
            u32  RsdtAddress;
            u32  Length;
            u64  XsdtAddress;
            u8   ExtendedChecksum;
            u8   Reserved[3];
        } __attribute__((packed));
        struct RSDT
        {
            SDTHeader Header;
            u64       Sdts[];
        } __attribute__((packed));

        bool  s_XsdtAvailable = false;
        RSDT* s_Rsdt          = nullptr;

        bool  ValidateChecksum(SDTHeader* header)
        {

            u32 checksum = 0;
            for (u32 i = 0; i < header->Length; i++)
                checksum += reinterpret_cast<u8*>(header)[i];

            return checksum;
        }
        Pointer GetTablePointer(u8 index)
        {
            if (s_XsdtAvailable)
                return Pointer(s_Rsdt->Sdts[index]).ToHigherHalf();

            const u32* ptr = reinterpret_cast<u32*>(s_Rsdt->Sdts);
            return Pointer(ptr[index]).ToHigherHalf();
        }
        void DetectACPIEntries()
        {
            const usize entryCount = (s_Rsdt->Header.Length - sizeof(SDTHeader))
                                   / (s_XsdtAvailable ? 8 : 4);
            LogInfo("ACPI: Table count: {}", entryCount);

            char acpiSignature[5];
            acpiSignature[4] = '\0';

            LogTrace("ACPI: Available tables:");
            for (usize i = 0; i < entryCount; i++)
            {
                SDTHeader* header = GetTablePointer(i).As<SDTHeader>();
                if (!header || !(reinterpret_cast<char*>(header->Signature))
                    || header->Signature[0] == 0)
                    continue;
                std::memcpy(acpiSignature, header->Signature, 4);
                LogTrace("  -{}", acpiSignature);
            }
        }
    } // namespace

    void Initialize()
    {
        LogTrace("ACPI: Initializing...");
        constexpr const char RSDP_SIGNATURE[] = "RSD PTR";
        RSDP* rsdp = BootInfo::GetRSDPAddress().ToHigherHalf<RSDP*>();
        if (std::strncmp(rsdp->Signature, RSDP_SIGNATURE,
                         std::size(RSDP_SIGNATURE) - 1)
            != 0)
        {
            LogError("ACPI: Invalid RSDP signature!");
            return;
        }

        s_XsdtAvailable = rsdp->Revision >= 2 && rsdp->XsdtAddress != 0;
        Pointer rsdtPointer
            = s_XsdtAvailable ? rsdp->XsdtAddress : rsdp->RsdtAddress;

        s_Rsdt = rsdtPointer.ToHigherHalf<RSDT*>();
        Assert(s_Rsdt != nullptr);

        LogInfo("ACPI: Found {} at: {:#x}", s_XsdtAvailable ? "XSDT" : "RSDT",
                reinterpret_cast<uintptr_t>(s_Rsdt));
        DetectACPIEntries();

        LogInfo("ACPI: Initialized");
        MADT::Initialize();
    }
    SDTHeader* GetTable(const char* signature, usize index)
    {
        Assert(signature != nullptr);

        const usize entryCount = (s_Rsdt->Header.Length - sizeof(SDTHeader))
                               / (s_XsdtAvailable ? 8 : 4);
        for (usize i = 0; i < entryCount; i++)
        {
            SDTHeader* header = GetTablePointer(i).As<SDTHeader>();

            if (!header || !(reinterpret_cast<char*>(header->Signature)))
                continue;
            if (!ValidateChecksum(header)) continue;

            if (std::strncmp(header->Signature, signature, 4) == 0)
            {
                if (index == 0) return header;
                ++index;
            }
        }

        return nullptr;
    }
} // namespace ACPI
