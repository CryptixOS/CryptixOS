/*
 * Created by v1tr10l7 on 20.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <Prism/Memory/Endian.hpp>

#include <format>
#include <string_view>
#include <unordered_map>

namespace DeviceTree
{
    class Property
    {
      public:
        BigEndian<u32> m_Length;
        BigEndian<u32> m_NameOffset;
    };

    class Node
    {
      public:
        enum class Tag : u32
        {
            eBeginNode = 1,
            eEndNode   = 2,
            eProperty  = 3,
            eNop       = 4,
            eEnd       = 9,
        };

        Node(std::string_view name)
            : m_Name(name)
        {
        }
        void InsertNode(std::string_view name, Node* node)
        {
            m_Children[name] = node;
        }
        void InsertProperty(std::string_view name, Property* property)
        {
            m_Properties[name] = property;
        }

        void Print(u32 depth = 0)
        {
            LogMessage("- {}", m_Name);
            for (const auto& [name, property] : m_Properties)
            {
                for (usize i = 0; i < depth; i++) Logger::LogChar(' ');
                LogMessage("- {}\n", name);
            }
            for (const auto& [name, node] : m_Children)
                return node->Print(depth + 4);
        }

      private:
        std::string                                     m_Name;
        std::unordered_map<std::string_view, Node*>     m_Children;
        std::unordered_map<std::string_view, Property*> m_Properties;
    };
}; // namespace DeviceTree
