/*
 * Created by v1tr10l7 on 04.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Firmware/ACPI/Interpreter/NameSpace.hpp>
#include <Library/Logger.hpp>

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
