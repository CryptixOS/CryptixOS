/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Memory/VMM.hpp>
#include <Utility/Stacktrace.hpp>

#include <vector>

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
    struct SectionHeader
    {
        u32       name;
        u32       type;
        u64       flags;
        uintptr_t addr;
        u64       offset;
        u64       size;
        u32       link;
        u32       info;
        u64       alignment;
        u64       entrySize;
    } __attribute__((packed));

    enum class HeaderType : u32
    {
        eNone          = 0,
        eLoad          = 1,
        eDynamic       = 2,
        eInterp        = 3,
        eNote          = 4,
        eProgramHeader = 6,
    };
    enum class SectionType : u32
    {
        eNull        = 0,
        eProgBits    = 1,
        eSymbolTable = 2,
        eStringTable = 3,
        eRelA        = 4,
        eNoBits      = 8,
        eRel         = 9,
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
    struct Sym
    {
        u32 name;
        u8  info;
        u8  other;
        u16 sectionIndex;
        u64 value;
        u64 size;
    } __attribute__((packed));

    struct AuxiliaryVector
    {
        uintptr_t EntryPoint;
        uintptr_t ProgramHeadersAddress;
        usize     ProgramHeaderEntrySize;
        usize     ProgramHeaderCount;
    };
    class Image
    {
      public:
        bool             Load(std::string_view path, PageMap* pageMap,
                              uintptr_t loadBase = 0);

        inline uintptr_t GetEntryPoint() const { return auxv.EntryPoint; }
        inline uintptr_t GetAtPhdr() const
        {
            return auxv.ProgramHeadersAddress;
        }
        inline uintptr_t GetPhent() const
        {
            return auxv.ProgramHeaderEntrySize;
        }
        inline uintptr_t GetPhNum() const { return auxv.ProgramHeaderCount; }

        inline const std::vector<Symbol>& GetSymbols() const { return symbols; }
        inline std::string_view           GetLdPath() const { return ldPath; }

      private:
        u8*                        image = nullptr;
        Header                     header;
        std::vector<ProgramHeader> programHeaders;
        std::vector<SectionHeader> sections;
        AuxiliaryVector            auxv;

        std::optional<u64>         stringSectionIndex;
        std::optional<u64>         symbolSectionIndex;

        std::vector<Symbol>        symbols;
        std::string_view           ldPath;

        bool                       Parse();
        void                       LoadSymbols();

        template <typename T>
        void Read(T* buffer, isize offset, isize count = sizeof(T))
        {
            std::memcpy(buffer, image + offset, count);
        }
    };
}; // namespace ELF
