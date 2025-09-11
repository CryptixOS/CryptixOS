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
#include <Prism/Utility/Optional.hpp>

namespace DeviceTree
{
    class PropertyHeader
    {
      public:
        BigEndian<u32> m_Length;
        BigEndian<u32> m_NameOffset;
    };

    class Node;
    struct Register
    {
        Pointer Base   = nullptr;
        usize   Length = 0;
    };

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

        friend class Node;
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

        Node(Node* parent, StringView name)
            : m_Parent(parent)
            , m_Name(name)
        {
        }

        inline Node*           Parent() const { return m_Parent; }
        inline StringView      Name() const { return m_Name; }

        inline Optional<usize> ID() const { return m_ID; }
        inline StringView      Model() const { return m_Model; }

        inline usize AddressCellCount() const { return m_AddressCellCount; }
        inline usize SizeCellCount() const { return m_SizeCellCount; }

        inline bool  IsCompatible(StringView name) const
        {
            for (StringView compatible : m_CompatibleDrivers)
                if (compatible == name) return true;
            return false;
        }
        inline const Vector<Register>& Registers() const { return m_Registers; }

        inline auto                    begin() { return m_Children.begin(); }
        inline auto begin() const { return m_Children.begin(); }

        inline auto end() { return m_Children.end(); }
        inline auto end() const { return m_Children.end(); }

        void        InsertNode(StringView name, Node* node)
        {
            m_Children[name] = node;
        }

        void AddProperty(StringView name, u8* data, usize length);
        void InsertProperty(StringView name, Property* property);

        void Parse();
        void Print(u32 depth = 0);

      private:
        Node*                               m_Parent           = nullptr;
        String                              m_Name             = ""_sv;

        Optional<usize>                     m_ID               = NullOpt;
        String                              m_Model            = ""_sv;

        usize                               m_AddressCellCount = 2;
        usize                               m_SizeCellCount    = 1;

        Vector<String>                      m_CompatibleDrivers;
        Vector<Register>                    m_Registers;

        UnorderedMap<StringView, Node*>     m_Children;
        UnorderedMap<StringView, Property*> m_Properties;
    };
}; // namespace DeviceTree

template <>
struct fmt::formatter<DeviceTree::Register> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const DeviceTree::Register& reg, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format("{:#x}:{:#x}", reg.Base, reg.Length), ctx);
    }
};
