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
            u64       Sdts[];
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

    bool IsAvailable() { return BootInfo::GetRSDPAddress().operator bool(); }
    bool LoadTables()
    {
        using namespace std::literals;

        LogTrace("ACPI: Initializing...");
        constexpr std::string_view RSDP_SIGNATURE = "RSD PTR"sv;
        auto                       rsdpPointer    = BootInfo::GetRSDPAddress();
        if (!rsdpPointer) return false;

        RSDP* rsdp = rsdpPointer.ToHigherHalf<Pointer>().As<RSDP>();
        if (RSDP_SIGNATURE.compare(0, 7, rsdp->Signature) == 0)
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
        auto status = uacpi_initialize(0);

        if (uacpi_unlikely_error(status))
        {
            LogError("ACPI: Failed to enter acpi mode!");
            return;
        }
    }
    void LoadNameSpace()
    {
        auto status = uacpi_namespace_load();
        if (uacpi_unlikely_error(status))
        {
            LogError("ACPI: Failed to load namespace");
            return;
        }
        status = uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC);
        if (uacpi_unlikely_error(status))
        {
            LogError("ACPI: Failed to set uACPI interrupt model");
            return;
        }
        status = uacpi_namespace_initialize();
        if (uacpi_unlikely_error(status))
        {
            LogError("ACPI: Failed to initialize namespace");
            return;
        }
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

    IA32BootArchitectureFlags GetIA32BootArchitectureFlags()
    {
        return s_FADT ? s_FADT->IA32BootArchitectureFlags
                      : IA32BootArchitectureFlags(0);
    }

    void Reboot()
    {
        // TODO(v1tr10l7): ACPI::Reboot
        return;
        LogTrace("ACPI: Attempting to reboot...");
        LogInfo("");
    }

} // namespace ACPI
