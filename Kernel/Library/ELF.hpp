/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Stacktrace.hpp>
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <vector>

namespace ELF
{
    constexpr const char* MAGIC = "\177ELF";

    struct [[gnu::packed]] Header
    {
        u32 Magic;
        u8  Bitness;
        u8  Endianness;
        u8  HeaderVersion;
        u8  Abi;
        u64 Padding;
        u16 Type;
        u16 InstructionSet;
        u32 ElfVersion;
        u64 EntryPoint;
        u64 ProgramHeaderTableOffset;
        u64 SectionHeaderTableOffset;
        u32 Flags;
        u16 HeaderSize;
        u16 ProgramEntrySize;
        u16 ProgramEntryCount;
        u16 SectionEntrySize;
        u16 SectionEntryCount;
        u16 SectionNamesIndex;
    };
    struct [[gnu::packed]] SectionHeader
    {
        u32       Name;
        u32       Type;
        u64       Flags;
        uintptr_t Address;
        u64       Offset;
        u64       Size;
        u32       Link;
        u32       Info;
        u64       Alignment;
        u64       EntrySize;
    };

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

    struct [[gnu::packed]] ProgramHeader
    {
        HeaderType        Type;
        SegmentAttributes Attributes;
        u64               Offset;
        u64               VirtualAddress;
        u64               PhysicalAddress;
        u64               SegmentSizeInFile;
        u64               SegmentSizeInMemory;
        u64               Alignment;
    };
    struct [[gnu::packed]] Sym
    {
        u32 Name;
        u8  Info;
        u8  Other;
        u16 SectionIndex;
        u64 Value;
        u64 Size;
    };

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
        bool        Load(std::string_view path, PageMap* pageMap,
                         std::vector<VMM::Region>& addressSpace,
                         uintptr_t                 loadBase = 0);
        bool        Load(Pointer image, usize size);

        static bool LoadModules(const u64 sectionCount, SectionHeader* sections,
                                char* stringTable);

        inline uintptr_t GetEntryPoint() const
        {
            return m_AuxiliaryVector.EntryPoint;
        }
        inline uintptr_t GetAtPhdr() const
        {
            return m_AuxiliaryVector.ProgramHeadersAddress;
        }
        inline uintptr_t GetPhent() const
        {
            return m_AuxiliaryVector.ProgramHeaderEntrySize;
        }
        inline uintptr_t GetPhNum() const
        {
            return m_AuxiliaryVector.ProgramHeaderCount;
        }

        inline const std::vector<Symbol>& GetSymbols() const
        {
            return m_Symbols;
        }
        inline std::string_view GetLdPath() const { return m_LdPath; }

      private:
        u8*                        m_Image = nullptr;
        Header                     m_Header;
        std::vector<ProgramHeader> m_ProgramHeaders;
        std::vector<SectionHeader> m_Sections;
        AuxiliaryVector            m_AuxiliaryVector;

        std::optional<u64>         m_StringSectionIndex;
        std::optional<u64>         m_SymbolSectionIndex;
        u8*                        m_StringTable = nullptr;

        std::vector<Symbol>        m_Symbols;
        std::string_view           m_LdPath;

        bool                       Parse();
        void                       LoadSymbols();

        template <typename T>
        void Read(T* buffer, isize offset, isize count = sizeof(T))
        {
            std::memcpy(buffer, m_Image + offset, count);
        }
    };
}; // namespace ELF
