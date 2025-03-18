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

// FDT property structure
struct fdt_property
{
    uint32_t len;
    uint32_t nameoff;
    // uint8_t data[]; // Flexible array member
};

// Function to convert big-endian to host-endian
static uint32_t fdt32_to_cpu(uint32_t x) { return __builtin_bswap32(x); }

// Function to align offset to 4 bytes
static uint32_t fdt_align_offset(uint32_t offset) { return (offset + 3) & ~3; }

// Function to get a string from the strings block
static const char* fdt_get_string(const char* strings, uint32_t stroffset)
{
    return strings + stroffset;
}

// Function to iterate through FDT nodes
void iterate_fdt_nodes(const void* fdt)
{
    const struct FDT_Header* header = (const struct FDT_Header*)fdt;
    const char*              struct_block
        = (const char*)fdt + fdt32_to_cpu(header->StructureBlockOffset.Value);
    const char* strings_block
        = (const char*)fdt + fdt32_to_cpu(header->StringBlockOffset.Value);
    uint32_t             offset = 0;
    [[maybe_unused]] int depth  = 0;

    while (offset < fdt32_to_cpu(header->StructureBlockLength.Value))
    {
        uint32_t token = fdt32_to_cpu(*(uint32_t*)(struct_block + offset));
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
                const struct fdt_property* prop
                    = (const struct fdt_property*)(struct_block + offset);
                const char* prop_name = fdt_get_string(
                    strings_block, fdt32_to_cpu(prop->nameoff));
                Logger::Logf(LogLevel::eTrace, "Property: %s\n", prop_name);
                offset += sizeof(struct fdt_property) + fdt32_to_cpu(prop->len);
                offset = fdt_align_offset(offset);
                break;
            }
            case FDT_NOP:
                // No operation; skip
                break;
            case FDT_END: return;
            default:
                Logger::Logf(LogLevel::eTrace, "Unknown token: 0x%x\n", token);
                return;
        }
    }
}

bool DeviceTree::Initialize()
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
    LogTrace("DeviceTree: Magic: {:#x}", magic.Load());
    LogTrace("DeviceTree: Magic: {}", magic == 0xd00dfeed);
    iterate_fdt_nodes(header);

    return magic == 0xd00dfeed;
}
