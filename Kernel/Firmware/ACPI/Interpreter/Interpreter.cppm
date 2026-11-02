/*
 * Created by v1tr10l7 on 28.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Compiler.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Memory/ByteStream.hpp>

export module Cryptix:Firmware.ACPI.Interpreter.Interpreter;

export import :Firmware.ACPI.Interpreter.NameSpace;
export import :Firmware.ACPI.Interpreter.OpCodes;
export import :Firmware.ACPI.Structures;

export namespace ACPI::Interpreter
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
