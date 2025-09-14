/*
 * Created by v1tr10l7 on 14.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Debug.hpp>

#include <Firmware/DeviceTree/DeviceTree.hpp>
#include <Library/Logger.hpp>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Math.hpp>

namespace DeviceTree
{
    static Node* s_RootNode = nullptr;

    bool         ParseFDT(FDT_Header* header)
    {
        char* structureBlock = reinterpret_cast<char*>(header)
                             + header->StructureBlockOffset.Load();
        char* stringBlock = reinterpret_cast<char*>(header)
                          + header->StringBlockOffset.Load();
        usize structureBlockSize = header->StructureBlockLength.Load();

        u64   offset             = 0;
        Node* root               = nullptr;
        Node* current            = nullptr;
        while (offset < structureBlockSize)
        {
            BigEndian<FDT_TokenType> tokenType
                = *reinterpret_cast<FDT_TokenType*>(structureBlock + offset);
            offset += sizeof(u32);

            switch (tokenType.Load())
            {
                case FDT_TokenType::eBeginNode:
                {
                    StringView name = structureBlock + offset;
                    offset
                        = Math::AlignUp(offset + name.Size() + 1, sizeof(u32));

                    auto* newNode = new Node(current, !root ? "/" : name);
                    if (!root) root = current;
                    else current->InsertNode(name, newNode);

                    current = newNode;
                    break;
                }
                case FDT_TokenType::eEndNode:
                    current->Parse();
                    current = (current && current->Parent()) ? current->Parent()
                                                             : root;
                    break;
                case FDT_TokenType::eProperty:
                {
                    auto* propertyHeader = reinterpret_cast<PropertyHeader*>(
                        structureBlock + offset);

                    StringView propertyName
                        = stringBlock + propertyHeader->m_NameOffset.Load();
                    u8* propertyData
                        = reinterpret_cast<u8*>(propertyHeader + 1);
                    u32 propertyDataSize = propertyHeader->m_Length.Load();

                    offset += sizeof(PropertyHeader) + propertyDataSize;
                    offset = Math::AlignUp(offset, 4);

                    if (current)
                        current->AddProperty(propertyName, propertyData,
                                             propertyDataSize);
                    break;
                }
                case FDT_TokenType::eNop:
                    // No operation; skip
                    break;
                case FDT_TokenType::eEnd: s_RootNode = root; return true;
                default:
                    EarlyLogError("DeviceTree: Unknown token: %#x",
                                  tokenType.Load());
                    return false;
            }
        }

        s_RootNode = root;
        return true;
    }
    bool ParseFDT(FDT_Header* header);

    bool Initialize(Pointer dtb)
    {
        FDT_Header* header = dtb.ToHigherHalf<FDT_Header*>();

        if (!header)
#ifdef CTOS_TARGET_AARCH64
            AssertMsg(header, "DeviceTree: Cannot boot without FDT header");
#else
            return false;
#endif

        auto magic = header->Magic;
        if (magic.Load() != FDT_MAGIC)
        {
            LogError("DeviceTree: FDT has an invalid magic: {:#x}",
                     magic.Load());
            return false;
        }

        auto success = ParseFDT(header);
        if (!success) return false;

#undef CTOS_DEVICE_TREE_DUMP
#define CTOS_DEVICE_TREE_DUMP 1
#if CTOS_DEVICE_TREE_DUMP != 0
        s_RootNode->Print();
#endif

        LogInfo("DeviceTree: FDT parsed successfully");
        return success;
    }

    static Node* FindCompatibleNode(const Driver& driver, Node& node)
    {
        if (node.IsCompatible(driver.Compatible)) return &node;

        for (auto& [name, child] : node)
        {
            auto found = FindCompatibleNode(driver, *child);
            if (found) return found;
        }

        return nullptr;
    }
    bool RegisterDriver(const Driver& driver)
    {
        auto found = FindCompatibleNode(driver, *s_RootNode);
        if (!found) return false;

        if (!driver.Probe) return false;
        auto success = driver.Probe(*found);

        if (!success) return false;
        return true;
    }
} // namespace DeviceTree
