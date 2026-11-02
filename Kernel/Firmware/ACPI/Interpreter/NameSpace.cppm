/*
 * Created by v1tr10l7 on 04.07.2025.
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

export module Cryptix:Firmware.ACPI.Interpreter.NameSpace;

export import :Firmware.ACPI.Interpreter.Object;

export namespace ACPI
{
    class NameSpace
    {
      public:
        explicit NameSpace(StringView name, NameSpace* parent = nullptr);
        void                                 Dump(usize tabs = 0);

        NameSpace*                           Insert(StringView name);

        NameSpace*                           m_Parent = nullptr;
        String                               m_Name   = "\\";
        UnorderedMap<StringView, NameSpace*> m_Children;
        Object*                              m_Object = nullptr;
        ObjectType                           m_Type   = ObjectType::eUndefined;
    };

    struct ControlMethod
    {
        // ByteStream<Endian::eLittle> Stream;
    };
}; // namespace ACPI
