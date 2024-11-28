/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

namespace ELF
{
    constexpr const char* MAGIC = "\177ELF";

    struct Header
    {
        u32 magic;
        u8  bitness;
        u8  endianness;
        u8  headerVersion;
        u8  abi;
        u64 padding;
        u16 type;
        u16 instructionSet;
        u32 elfVersion;
        u64 entryPoint;
        u64 programHeaderTableOffset;
        u64 sectionHeaderTableOffset;
        u32 flags;
        u16 headerSize;
        u16 programEntrySize;
        u16 programEntryCount;
        u16 sectionEntrySize;
        u16 sectionEntryCount;
        u16 sectionNamesIndex;
    } __attribute__((packed));
    enum class HeaderType : u32
    {
        eNone        = 0,
        eLoad        = 1,
        eDynamic     = 2,
        eLinking     = 3,
        eInterp      = 4,
        eNoteSection = 5,
    };
    enum class SegmentAttributes : u32
    {
        eExecutable = 1,
        eWriteable  = 2,
        eReadable   = 4,
    };

    struct ProgramHeader
    {
        HeaderType        type;
        SegmentAttributes attributes;
        u64               offset;
        u64               virtualAddress;
        u64               physicalAddress;
        u64               segmentSizeInFile;
        u64               segmentSizeInMemory;
        u64               alignment;

    } __attribute__((packed));
    class Image
    {
      public:
        uintptr_t Load(std::string_view path);

      private:
        Header header;
    };
}; // namespace ELF
