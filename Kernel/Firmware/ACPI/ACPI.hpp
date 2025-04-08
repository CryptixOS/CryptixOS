/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Firmware/ACPI/Structures.hpp>

namespace ACPI
{
    bool       IsAvailable();

    bool       LoadTables();
    void       Enable();

    void       LoadNameSpace();
    void       EnumerateDevices();

    SDTHeader* GetTable(const char* signature, usize index = 0);
    template <typename T>
    inline T* GetTable(const char* signature, usize index = 0)
    {
        return reinterpret_cast<T*>(GetTable(signature, index));
    }

    X86BootArchitectureFlags GetX86BootArchitectureFlags();
    usize                    GetCentury();

    void                     Reboot();
} // namespace ACPI
