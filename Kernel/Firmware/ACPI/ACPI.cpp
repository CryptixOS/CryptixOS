/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/ACPI/MADT.hpp>

#include <Boot/BootInfo.hpp>
#include <Memory/ScopedMapping.hpp>
#include <Memory/VMM.hpp>

#include <uacpi/context.h>
#include <uacpi/event.h>
#include <uacpi/resources.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

#define uAcpiCall(cmd, ...)                                                    \
    {                                                                          \
        auto status = (cmd);                                                   \
        if (uacpi_unlikely_error(status))                                      \
        {                                                                      \
            LogError(__VA_ARGS__);                                             \
            return;                                                            \
        }                                                                      \
    }

namespace ACPI
{
    namespace
    {
        struct [[gnu::packed]] RSDP
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
        };
        struct [[gnu::packed]] RSDT
        {
            SDTHeader Header;
            union
            {
                u32 Sdts32[];
                u64 Sdts64[];
            };
        };

        bool  s_XsdtAvailable = false;
        RSDT* s_Rsdt          = nullptr;
        FADT* s_FADT          = nullptr;

        bool  ValidateChecksum(SDTHeader* header)
        {

            u32 checksum = 0;
            for (u32 i = 0; i < header->Length; i++)
                checksum += reinterpret_cast<u8*>(header)[i];

            return checksum;
        }
        Pointer GetTablePointer(u8 index)
        {
            Pointer tableAddress = s_XsdtAvailable ? s_Rsdt->Sdts64[index]
                                                   : s_Rsdt->Sdts32[index];

            return tableAddress.ToHigherHalf();
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
        void InitializeFADT(Pointer fadtAddress)
        {
            ScopedMapping<FADT> fadt(fadtAddress,
                                     PageAttributes::eRW
                                         | PageAttributes::eUncacheableStrong);

            if (fadt->Header.Revision <= 2)
            {
                fadt->PreferredPmProfile = 0;
                fadt->PStateControl      = 0;
                fadt->CStateControl      = 0;
                fadt->X86BootArchitectureFlags
                    = static_cast<X86BootArchitectureFlags>(0);
            }
            if (!fadt->X_Dsdt) fadt->X_Dsdt = fadt->DSDT;
            LogTrace("ACPI: Found DSDT table");
        }

    } // namespace

    bool IsAvailable() { return BootInfo::GetRSDPAddress().operator bool(); }
    bool LoadTables()
    {
        using namespace std::literals;

        LogTrace("ACPI: Initializing...");
        constexpr StringView RSDP_SIGNATURE = "RSD PTR"_sv;
        auto                 rsdpPointer    = BootInfo::GetRSDPAddress();
        if (!rsdpPointer) return false;

        RSDP* rsdp = rsdpPointer.ToHigherHalf<Pointer>().As<RSDP>();
        if (RSDP_SIGNATURE.Compare(0, 7, rsdp->Signature) == 0)
        {
            LogError("ACPI: Invalid RSDP signature!");
            return false;
        }

        s_XsdtAvailable = rsdp->Revision >= 2 && rsdp->XsdtAddress != 0;
        Pointer rsdtPointer
            = s_XsdtAvailable ? rsdp->XsdtAddress : rsdp->RsdtAddress;

        if (!rsdtPointer) return false;
        s_Rsdt = rsdtPointer.ToHigherHalf<RSDT*>();

        LogInfo("ACPI: Found {} at: {:#x}", s_XsdtAvailable ? "XSDT" : "RSDT",
                reinterpret_cast<uintptr_t>(s_Rsdt));
        DetectACPIEntries();

        MADT::Initialize();
        s_FADT = GetTable<FADT>("FACP");
        InitializeFADT(Pointer(s_FADT).FromHigherHalf());

        LogInfo("ACPI: Loaded static tables");
        return true;
    }

    static uacpi_iteration_decision
    EnumerateDevice(void* ctx, uacpi_namespace_node* node, uacpi_u32 node_depth)
    {
        uacpi_namespace_node_info* info;
        (void)node_depth;

        uacpi_status ret = uacpi_get_namespace_node_info(node, &info);
        if (uacpi_unlikely_error(ret))
        {
            const char* path
                = uacpi_namespace_node_generate_absolute_path(node);
            LogError("ACPI: Unable to retrieve node %s information: {}", path,
                     uacpi_status_to_string(ret));
            uacpi_free_absolute_path(path);
            return UACPI_ITERATION_DECISION_CONTINUE;
        }

        LogInfo("ACPI: Found device - {}", info->name.text);
        uacpi_free_namespace_node_info(info);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }
    void Enable()
    {
        LogTrace("ACPI: Entering ACPI Mode");

#define UACPI_DEBUG 0
#if UACPI_DEBUG == 1
        uacpi_context_set_log_level(UACPI_LOG_DEBUG);
#else
        uacpi_context_set_log_level(UACPI_LOG_INFO);
#endif

        uAcpiCall(uacpi_initialize(0), "ACPI: Failed to enter acpi mode!");
    }
    void LoadNameSpace()
    {
        uAcpiCall(uacpi_namespace_load(), "ACPI: Failed to load namespace");
        uAcpiCall(uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC),
                  "ACPI: Failed to set uACPI interrupt model");
        uAcpiCall(uacpi_namespace_initialize(),
                  "ACPI: Failed to initialize namespace");
        // uAcpiCall(uacpi_finalize_gpe_initialization(),
        //           "ACPI: Failed to finalize gpe initialization!");
    }
    void EnumerateDevices()
    {
        uacpi_namespace_for_each_child(uacpi_namespace_root(), EnumerateDevice,
                                       UACPI_NULL, UACPI_OBJECT_DEVICE_BIT,
                                       UACPI_MAX_DEPTH_ANY, UACPI_NULL);
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

            if (std::strncmp(signature, header->Signature, 4) == 0)
            {
                if (index == 0) return header;
                ++index;
            }
        }

        return nullptr;
    }

    X86BootArchitectureFlags GetX86BootArchitectureFlags()
    {
        return s_FADT ? s_FADT->X86BootArchitectureFlags
                      : X86BootArchitectureFlags();
    }
    usize GetCentury() { return s_FADT ? s_FADT->Century : 0; }

    void  Reboot()
    {
        if (!s_FADT) return;
        LogTrace("ACPI: Attempting to reboot...");

        s_FADT->ResetReg.Write(s_FADT->ResetValue);
    }
} // namespace ACPI
