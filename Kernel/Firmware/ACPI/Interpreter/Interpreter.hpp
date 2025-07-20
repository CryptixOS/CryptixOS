/*
 * Created by v1tr10l7 on 28.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Firmware/ACPI/Interpreter/NameSpace.hpp>
#include <Firmware/ACPI/Interpreter/OpCodes.hpp>
#include <Firmware/ACPI/Structures.hpp>

#include <Prism/Memory/ByteStream.hpp>

namespace ACPI::Interpreter
{
    struct ExecutionContext
    {
        ByteStream<Endian::eLittle> Stream;
        u16                         CurrentOpCode = 0;
    };
    struct CodeBlock
    {
        NameSpace* NameSpace     = nullptr;
        usize      Start         = 0;
        usize      End           = 0;

        u16        OpCode        = 0;
        String     Name          = ""_sv;
        usize      PackageLength = 0;
    };

    void ExecuteTable(ACPI::SDTHeader& table);
}; // namespace ACPI::Interpreter
