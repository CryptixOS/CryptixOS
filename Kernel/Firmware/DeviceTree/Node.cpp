/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Assertions.hpp>
#include <Firmware/DeviceTree/Node.hpp>

namespace DeviceTree
{
    Property::Property(Node* parent, StringView name, u8* data, usize dataSize)
        : m_Parent(parent)
        , m_Name(name)
        , m_Data(data)
        , m_DataSize(dataSize)
    {
    }

    void Property::Print(usize depth)
    {
        for (usize i = 0; i < depth; i++) Logger::LogChar(' ');
        LogMessage("- {} ", m_Name);

        if (m_DataSize == 4)
        {
            BigEndian<u32> value = *reinterpret_cast<u32*>(m_Data);
            LogMessage("Value (u32): {:#x} ({})", value.Load(), value.Load());
        }
        else if (m_DataSize > 0 && m_Data[m_DataSize - 1] == 0)
            LogMessage("Value (string): '{}'",
                       reinterpret_cast<const char*>(m_Data));
        else
        {
            LogMessage(" Value (raw): \n");
            for (u32 i = 0; i < m_DataSize; i++)
                Logger::Log(LogLevel::eNone,
                            fmt::format("{:02x}", m_Data[i]).data(), false);
        }
        Logger::Print("\n");
    }

    void Node::AddProperty(StringView name, u8* data, usize length)
    {
        Property* property = new Property(this, name, data, length);
        InsertProperty(name, property);

        if (name == "phandle"_sv)
        {
            Assert(length == 4);
            m_ID = *reinterpret_cast<u32*>(data);
        }
        else if (name == "compatible"_sv)
        {
            StringView compatible(reinterpret_cast<char*>(data));

            m_CompatibleDrivers = compatible.Split(',');
        }
        else if (name == "#size-cells"_sv)
            m_SizeCellCount = *reinterpret_cast<u32*>(data);
        else if (name == "#address-cells"_sv)
            m_AddressCellCount = *reinterpret_cast<u32*>(data);
    }
    void Node::InsertProperty(StringView name, Property* property)
    {
        m_Properties[name] = property;
    }

    void Node::Print(u32 depth)
    {
        LogMessage("- {}\n", m_Name);
        for (auto& [name, property] : m_Properties) property->Print(depth);

        for (const auto& [name, node] : m_Children) node->Print(depth + 4);
    }
}; // namespace DeviceTree
