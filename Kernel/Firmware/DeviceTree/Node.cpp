/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Assertions.hpp>
#include <Firmware/DeviceTree/Node.hpp>
#include <Prism/String/Formatter.hpp>

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
                Logger::Print(fmt::format("{:02x}", m_Data[i]).data());
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
            m_SizeCellCount
                = BigEndian<u32>(*reinterpret_cast<u32*>(data)).Load();
        else if (name == "#address-cells"_sv)
            m_AddressCellCount
                = BigEndian<u32>(*reinterpret_cast<u32*>(data)).Load();
        else if (name == "model"_sv) m_Model = reinterpret_cast<char*>(data);
    }
    void Node::InsertProperty(StringView name, Property* property)
    {
        m_Properties[name] = property;
    }

    void Node::Parse()
    {
        auto sizeCellCount    = SizeCellCount();
        auto addressCellCount = AddressCellCount();

        auto regIt            = m_Properties.Find("reg");
        if (regIt == m_Properties.end()) return;

        u8*       dataStart  = regIt->Value->m_Data;
        usize     dataLength = regIt->Value->m_DataSize;
        const u8* dataEnd    = dataStart + dataLength;
        i32       entryCells = addressCellCount + sizeCellCount;
        i32       entryBytes = entryCells * 4;

        auto      current    = dataStart;
        while (current + entryBytes <= dataEnd)
        {
            u64 addr = 0;
            u64 size = 0;

            // Parse address
            for (usize i = 0; i < addressCellCount; i++)
            {
                addr = (addr << 32)
                     | BigEndian<u32>(*reinterpret_cast<u32*>(current)).Load();
                current += 4;
            }

            // Parse size
            for (usize i = 0; i < sizeCellCount; i++)
            {
                size = (size << 32)
                     | BigEndian<u32>(*reinterpret_cast<u32*>(current)).Load();
                current += 4;
            }

            Register reg = {addr, size};
            m_Registers.PushBack(reg);
        }
    }
    void Node::Print(u32 depth)
    {
        LogMessage("- {}\n", m_Name);

        String spaces;
        for (usize i = 0; i < depth; i++) spaces += ' ';
        for (auto& [name, property] : m_Properties)
        {
            if (name == "phandle"_sv)
                Logger::Print(
                    fmt::format("{}- phandle: {:#08x}\n", spaces, m_ID.Value())
                        .data());
            else if (name == "model"_sv)
                Logger::Print(
                    fmt::format("{}- model: {}\n", spaces, m_Model).data());
            else if (name == "#address-cells"_sv)
                Logger::Print(fmt::format("{}- #address-cells: {}\n", spaces,
                                          m_AddressCellCount)
                                  .data());
            else if (name == "#size-cells"_sv)
                Logger::Print(fmt::format("{}- #size-cells: {}\n", spaces,
                                          m_SizeCellCount)
                                  .data());
            else if (name == "compatible"_sv)
            {
                Logger::Print(fmt::format("{}- compatible: ", spaces).data());
                for (StringView compatible : m_CompatibleDrivers)
                    Logger::Print(fmt::format("{} ", compatible).data());
                Logger::LogChar('\n');
            }
            else if (name == "reg"_sv)
            {
                Logger::Print(fmt::format("{}- reg: ", spaces).data());
                for (auto& reg : m_Registers)
                    Logger::Print(
                        fmt::format("{:#08x}:{:#08x} | ", reg.Base, reg.Length)
                            .data());
                Logger::LogChar('\n');
            }
            else property->Print(depth);
        }

        for (const auto& [name, node] : m_Children) node->Print(depth + 4);
    }
}; // namespace DeviceTree
