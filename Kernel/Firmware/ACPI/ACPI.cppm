/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Compiler.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringUtils.hpp>

export module Cryptix:Firmware.ACPI.ACPI;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :Firmware.ACPI.Structures;

export namespace ACPI
{
    bool       IsAvailable();
    Pointer    RSDP();

    bool       LoadTables(Pointer rsdp);
    void       Enable();

    void       LoadNameSpace();
    void       EnumerateDevices();

    SDTHeader* GetTable(StringView signature, usize index = 0);
    template <typename T>
    inline T* GetTable(StringView signature, usize index = 0)
    {
        return reinterpret_cast<T*>(GetTable(signature, index));
    }

    X86BootArchitectureFlags GetX86BootArchitectureFlags();
    usize                    GetCentury();

    void                     Reboot();
} // namespace ACPI
