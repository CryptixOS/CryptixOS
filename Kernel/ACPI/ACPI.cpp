/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ACPI.hpp"

#include "Utility/BootInfo.hpp"

#include "ACPI/MADT.hpp"
#include "Memory/VMM.hpp"

namespace ACPI
{
    namespace
    {
#pragma pack(push, 1)
        struct RSDP
        {
            char signature[8];
            u8   checksum;
            char oemID[6];
            u8   revision;
            u32  rsdtAddress;
            u32  length;
            u64  xsdtAddress;
            u8   extendedChecksum;
            u8   reserved[3];
        };
        struct RSDT
        {
            SDTHeader header;
            u64       sdts[];
        };
#pragma pack(pop)

        bool  xsdt = false;
        RSDT* rsdt;

        bool  ValidateChecksum(SDTHeader* header)
        {

            u32 checksum = 0;
            for (u32 i = 0; i < header->length; i++)
                checksum += reinterpret_cast<char*>(header)[i];

            return checksum;
        }
        uintptr_t GetTablePointer(u8 index)
        {
            if (xsdt) return rsdt->sdts[index] + BootInfo::GetHHDMOffset();

            const u32* ptr = reinterpret_cast<u32*>(rsdt->sdts);
            return static_cast<uintptr_t>(ptr[index])
                 + BootInfo::GetHHDMOffset();
        }
        void DetectACPIEntries()
        {
            const usize entryCount
                = (rsdt->header.length - sizeof(SDTHeader)) / (xsdt ? 8 : 4);
            LogInfo("ACPI: Tables count: {}", entryCount);

            char acpiSignature[5];
            acpiSignature[4] = '\0';

            LogTrace("ACPI: Printing all tables signatures...");
            for (usize i = 0; i < entryCount; i++)
            {
                SDTHeader* header
                    = reinterpret_cast<SDTHeader*>(GetTablePointer(i));
                if (!header || !((char*)header->signature)
                    || header->signature[0] == 0)
                    continue;
                memcpy(acpiSignature, header->signature, 4);
                LogTrace("  -{}", acpiSignature);
            }
        }
    } // namespace

    void Initialize()
    {
        LogTrace("ACPI: Initializing...");
        RSDP* rsdp = ToHigherHalfAddress<RSDP*>(BootInfo::GetRSDPAddress());
        xsdt       = rsdp->revision >= 2 && rsdp->xsdtAddress != 0;
        u64 rsdtPointer = xsdt ? rsdp->xsdtAddress : rsdp->rsdtAddress;

        rsdt            = ToHigherHalfAddress<RSDT*>(rsdtPointer);
        Assert(rsdt != nullptr);

        LogInfo("ACPI: Found {} at: {:#x}", xsdt ? "XSDT" : "RSDT",
                reinterpret_cast<uintptr_t>(rsdt));
        DetectACPIEntries();

        LogTrace("ACPI: Initialized");
        MADT::Initialize();
    }
    SDTHeader* GetTable(const char* signature, usize index)
    {
        Assert(signature != nullptr);

        const usize entryCount
            = (rsdt->header.length - sizeof(SDTHeader)) / (xsdt ? 8 : 4);
        for (usize i = 0; i < entryCount; i++)
        {
            SDTHeader* header
                = reinterpret_cast<SDTHeader*>(GetTablePointer(i));

            if (!header || !((char*)header->signature)) continue;
            if (!ValidateChecksum(header)) continue;

            if (strncmp(header->signature, signature, 4) == 0)
            {
                if (index == 0) return header;
                ++index;
            }
        }

        return nullptr;
    }
} // namespace ACPI
