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

module Cryptix:Impl.Firmware.ACPI.Interpreter.NameSpace;

import :Firmware.ACPI.Interpreter.NameSpace;
import :Library.Logger;

namespace ACPI
{
    NameSpace::NameSpace(StringView name, NameSpace* parent)
        : m_Parent(parent), m_Name(name)
    {
    }

    NameSpace* NameSpace::Insert(StringView name)
    {
        auto newNameSpace = new NameSpace(name, this);
        m_Children[name] = newNameSpace;

        return newNameSpace;
    }

    void NameSpace::Dump(usize tabs)
    {
        for (usize i = 0; i < tabs; i += 4)
            LogMessage("\t");
        LogMessage("{} =>\n", m_Name);
        
        for (const auto& [name, child] : m_Children)
            child->Dump(tabs + 4);
    }
};
