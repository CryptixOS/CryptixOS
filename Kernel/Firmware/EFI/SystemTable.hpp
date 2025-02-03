/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/EFI/Types.hpp>

using EFI_SIMPLE_TEXT_INPUT_PROTOCOL  = void;
using EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL = void;
using EFI_RUNTIME_SERVICES            = void;
using EFI_BOOT_SERVICES               = void;
using EFI_CONFIGURATION_TABLE         = void;

struct SystemTable
{
    TableHeader                      Hdr;
    char16_t*                        FirmwareVendor;
    u32                              FirmwareRevision;
    EFI_HANDLE                       ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL*  ConIn;
    EFI_HANDLE                       ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE                       StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    EFI_RUNTIME_SERVICES*            RuntimeServices;
    EFI_BOOT_SERVICES*               BootServices;
    usize                            NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE*         ConfigurationTable;
};
