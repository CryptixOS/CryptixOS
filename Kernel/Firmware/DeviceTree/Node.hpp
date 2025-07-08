/*
 * Created by v1tr10l7 on 20.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/String.hpp>

namespace DeviceTree
{
    class PropertyHeader
    {
      public:
        BigEndian<u32> m_Length;
        BigEndian<u32> m_NameOffset;
    };

    class Node;
    class Property
    {
      public:
        Property() = default;
        Property(Node* parent, StringView name, u8* data, usize dataSize);

        inline Node* Parent() const { return m_Parent; }

        void         Print(usize depth);

      private:
        Node*  m_Parent   = nullptr;
        String m_Name     = "";
        u8*    m_Data     = nullptr;
        usize  m_DataSize = 0;
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

        Node*      Parent() { return m_Parent; }
        StringView Name() { return m_Name; }

        Node(Node* parent, StringView name)
            : m_Parent(parent)
            , m_Name(name)
        {
        }
        void InsertNode(StringView name, Node* node)
        {
            m_Children[name] = node;
        }
        void InsertProperty(StringView name, Property* property);

        void Print(u32 depth = 0);

      private:
        Node*                               m_Parent = nullptr;
        String                              m_Name;

        UnorderedMap<StringView, Node*>     m_Children;
        UnorderedMap<StringView, Property*> m_Properties;
    };
}; // namespace DeviceTree
