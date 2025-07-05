/*
 * Created by v1tr10l7 on 04.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>
#include <Firmware/ACPI/Interpreter/Object.hpp>

#include <unordered_map>
#include <magic_enum/magic_enum.hpp>

namespace ACPI
{
    class NameSpace 
    {
      public:  
        NameSpace(StringView name, NameSpace* parent = nullptr);
        void Dump(usize tabs = 0);

        NameSpace* Insert(StringView name);

        NameSpace* m_Parent = nullptr;
        String m_Name = "\\";
        std::unordered_map<StringView, NameSpace*> m_Children;
        Object* m_Object = nullptr;
        ObjectType m_Type = ObjectType::eUndefined;
    };
};
