/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Firmware/EFI/Types.hpp>
#include <Prism/Core/Types.hpp>

export module Cryptix:Firmware.EFI.SystemTable;

export import :Firmware.EFI.RuntimeServices;
export import :Firmware.EFI.Types;

export using EFI_SIMPLE_TEXT_INPUT_PROTOCOL  = void;
export using EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL = void;
export using EFI_BOOT_SERVICES               = void;
export using EFI_CONFIGURATION_TABLE         = void;

export namespace EFI
{
    struct ConfigurationTable
    {
        Guid  VendorGuid;
        Void* VendorTable;
    };
    struct SystemTable
    {
        TableHeader                      Hdr;
        Char16*                          FirmwareVendor;
        Uint32                           FirmwareRevision;
        Handle                           ConsoleInHandle;
        EFI_SIMPLE_TEXT_INPUT_PROTOCOL*  ConIn;
        Handle                           ConsoleOutHandle;
        EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
        Handle                           StandardErrorHandle;
        EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;

        RuntimeServices*                 RuntimeServices;
        EFI_BOOT_SERVICES*               BootServices;

        Size                             NumberOfTableEntries;
        ConfigurationTable*              ConfigurationTable;
    };

    extern SystemTable* g_SystemTable;
}; // namespace EFI
