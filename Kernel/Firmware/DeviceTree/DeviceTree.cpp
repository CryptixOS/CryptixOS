/*
 * Created by v1tr10l7 on 14.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Firmware/DeviceTree/DeviceTree.hpp>
#include <Library/Logger.hpp>

#include <Prism/Debug/Assertions.hpp>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// FDT token definitions
#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE   0x2
#define FDT_PROP       0x3
#define FDT_NOP        0x4
#define FDT_END        0x9

namespace DeviceTree
{
    // Function to convert big-endian to host-endian
    static uint32_t fdt32_to_cpu(uint32_t x) { return __builtin_bswap32(x); }

    // Function to align offset to 4 bytes
    static uint32_t fdt_align_offset(uint32_t offset)
    {
        return (offset + 3) & ~3;
    }

    // Function to get a string from the strings block
    static const char* fdt_get_string(const char* strings, uint32_t stroffset)
    {
        return strings + stroffset;
    }

    // Function to iterate through FDT nodes
    bool ParseFDT(FDT_Header* header)
    {
        char* struct_block
            = (char*)header + header->StructureBlockOffset.Load();
        char* strings_block = (char*)header + header->StringBlockOffset.Load();
        uint32_t             offset = 0;
        [[maybe_unused]] int depth  = 0;

        while (offset < header->StructureBlockLength.Load())
        {
            BigEndian<u32> tokenOffset
                = *reinterpret_cast<u32*>(struct_block + offset);
            uint32_t token = tokenOffset.Load();
            offset += sizeof(uint32_t);

            switch (token)
            {
                case FDT_BEGIN_NODE:
                {
                    const char* name = struct_block + offset;
                    Logger::Logf(LogLevel::eTrace, "Node: %s\n", name);
                    offset += strlen(name) + 1;
                    offset = fdt_align_offset(offset);
                    depth++;
                    break;
                }
                case FDT_END_NODE: depth--; break;
                case FDT_PROP:
                {
                    Property*   prop      = (Property*)(struct_block + offset);
                    u32         propLen   = prop->m_Length.Load();
                    const char* prop_name = fdt_get_string(
                        strings_block, prop->m_NameOffset.Load());
                    u8* propData = (u8*)(prop + 1);

                    Logger::Logf(LogLevel::eTrace, "Property: %s (Length: %u)",
                                 prop_name, propLen);

                    if (propLen == 4)
                    {
                        u32 value
                            = fdt32_to_cpu(*reinterpret_cast<u32*>(propData));
                        LogTrace(" Value (u32): {:#x} ({})", value, value);
                    }
                    else if (propLen > 0 && propData[propLen - 1 == '\0'])
                        LogTrace(" Value (string): '{}'",
                                 (const char*)propData);
                    else
                    {
                        LogTrace(" Value (raw): ");
                        for (u32 i = 0; i < propLen; i++)
                            Logger::Log(LogLevel::eTrace,
                                        std::format("{:02x}", propData[i]),
                                        false);
                        Logger::Print("\n");
                    }

                    offset += sizeof(Property) + prop->m_Length.Load();
                    offset = fdt_align_offset(offset);
                    break;
                }
                case FDT_NOP:
                    // No operation; skip
                    break;
                case FDT_END: return true;
                default:
                    Logger::Logf(LogLevel::eTrace, "Unknown token: 0x%x\n",
                                 token);
                    return false;
            }
        }

        return true;
    }
    bool ParseFDT(FDT_Header* header);

    bool Initialize()
    {
        auto        dtb    = BootInfo::GetDeviceTreeBlobAddress();
        FDT_Header* header = dtb.As<FDT_Header>();

        if (!header)
#ifdef CTOS_TARGET_AARCH64
            AssertMsg(header, "AARCH64: Cannot boot without FDT header");
#else
            return false;
#endif

        auto magic = header->Magic;
        if (magic.Load() != FDT_MAGIC)
        {
            LogError("FDT: Invalid magic: {:#x}", magic.Load());
            return false;
        }

        return ParseFDT(header);
    }
} // namespace DeviceTree
