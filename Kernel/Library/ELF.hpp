/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Library/Stacktrace.hpp>

#include <Prism/Containers/DoublyLinkedList.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/ByteStream.hpp>
#include <Prism/PathView.hpp>

#include <unordered_map>

class AddressSpace;
class PageMap;
namespace ELF
{
    constexpr const char MAGIC[] = "\177ELF";
    enum class Bitness : u8
    {
        e32Bit = 0b01,
        e64Bit = 0b10,
    };
    enum class Endianness : u8
    {
        eLittle = 0b01,
        eBig    = 0b10,
    };

    constexpr usize CURRENT_ELF_HEADER_VERSION = 1;
    enum class ABI : u8
    {
        eSystemV            = 0x00,
        eHPUX               = 0x01,
        eNetBSD             = 0x02,
        eLinux              = 0x03,
        eGNUHurd            = 0x04,
        eSolaris            = 0x06,
        eAIX                = 0x07,
        eIRIX               = 0x08,
        eFreeBSD            = 0x09,
        eTru64              = 0x0a,
        eNovellModesto      = 0x0b,
        eOpenBSD            = 0x0c,
        eOpenVMS            = 0x0d,
        eNonStopKernel      = 0x0e,
        eAros               = 0x0f,
        eFenixOS            = 0x10,
        eNuxiCloudAbi       = 0x11,
        eStratusTechOpenVOS = 0x12,
    };
    enum class ObjectType : u8
    {
        eUnknown     = 0x00,
        eRelocatable = 0x01,
        eExecutable  = 0x02,
        eShared      = 0x03,
        eCore        = 0x04,
    };
    enum class InstructionSet : u16
    {
        eUnknown              = 0x00,
        eATnT                 = 0x01,
        eSPARC                = 0x02,
        eX86                  = 0x03,
        eMotorolaM68K         = 0x04,
        eMotorolaM88K         = 0x05,
        eIntelMCU             = 0x06,
        eIntel80860           = 0x07,
        eMips                 = 0x08,
        eIBMSystem370         = 0x09,
        eMipsRs3kLe           = 0x0a,
        eHewlettPackard       = 0x0f,
        eIntel80960           = 0x13,
        ePowerPC              = 0x14,
        ePowerPC64            = 0x15,
        eS390                 = 0x16,
        eIbmSpuSpc            = 0x17,
        eNecV800              = 0x24,
        eFujitsuFr20          = 0x25,
        eTrwRh32              = 0x26,
        eMotorolaRce          = 0x27,
        eArmv7                = 0x28,
        eDigitalAlpha         = 0x29,
        eSuperH               = 0x2a,
        eSparcV9              = 0x2b,
        eSiemiensTriCore      = 0x2c,
        eArgonautRiscCore     = 0x2d,
        eHitachiH8_300        = 0x2e,
        eHitachiH8_300H       = 0x2f,
        eHitachiH85           = 0x30,
        eHitachiH8_500        = 0x31,
        eIA64                 = 0x32,
        eStanfordMipsX        = 0x33,
        eMotorolaColdFire     = 0x34,
        eMotorolaM68HC12      = 0x35,
        eFujitsuMMA           = 0x36,
        eSiemensPCP           = 0x37,
        eSonyNCPUrisc         = 0x38,
        eDensoNDR1            = 0x39,
        eMotorolaStarCore     = 0x3a,
        eToyotaMe16           = 0x3b,
        eStmST100             = 0x3c,
        eAdvancedLogicCorp    = 0x3d,
        eAMDx86_64            = 0x3e,
        eSonyDSP              = 0x3f,
        eDigitalEqCorpPDP10   = 0x40,
        eDigitalEqCorpPDP11   = 0x41,
        eSiemensFX66          = 0x42,
        eSTMsT9_8_16          = 0x43,
        eSTMsT9_8             = 0x44,
        eMotorolaMC68HC16     = 0x45,
        eMotorolaMC68HC11     = 0x46,
        eMotorolaMC68HC08     = 0x47,
        eMotorolaMC68HC05     = 0x48,
        eSiliconGraphicsSVx   = 0x49,
        eStmST19              = 0x4a,
        eDigitalVAX           = 0x4b,
        eAxisComm32           = 0x4c,
        eInfineonTech32       = 0x4d,
        eElement14DSP         = 0x4e,
        eLSILogic16DSP        = 0x4f,
        eTMS320C6k            = 0x8c,
        eMCSTElbrusE2k        = 0xaf,
        eArm64                = 0xb7,
        eZilogZ80             = 0xdc,
        eRiscV                = 0xf3,
        eBerkeleyPacketFilter = 0xf7,
        eWDC65C816            = 0x101,
        eLoongArch            = 0x102,
    };

    constexpr usize VERSION = 1;

    struct [[gnu::packed]] Header
    {
        u32            Magic;
        Bitness        Bitness;
        Endianness     Endianness;
        u8             HeaderVersion;
        ABI            Abi;
        u64            Padding;
        u16            Type;
        InstructionSet InstructionSet;
        u32            ElfVersion;
        u64            EntryPoint;
        u64            ProgramHeaderTableOffset;
        u64            SectionHeaderTableOffset;
        u32            Flags;
        u16            HeaderSize;
        u16            ProgramEntrySize;
        u16            ProgramEntryCount;
        u16            SectionEntrySize;
        u16            SectionEntryCount;
        u16            SectionNamesIndex;
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
        eNone                   = 0,
        eLoad                   = 1,
        eDynamic                = 2,
        eInterp                 = 3,
        eNote                   = 4,
        eReserved               = 5,
        eProgramHeader          = 6,
        eTLS                    = 7,
        eOsSpecificStart        = 0x60000000,
        eOsSpecificEnd          = 0x6fffffff,
        eProcessorSpecificStart = 0x70000000,
        eProcessorSpecificEnd   = 0x7fffffff,
    };
    enum class SectionType : u32
    {
        eNull                   = 0,
        eProgBits               = 1,
        eSymbolTable            = 2,
        eStringTable            = 3,
        eRelA                   = 4,
        eHash                   = 5,
        eDynamic                = 6,
        eNote                   = 7,
        eNoBits                 = 8,
        eRel                    = 9,
        eDynSym                 = 0x0b,
        eInitArray              = 0x0e,
        eFiniArray              = 0x0f,
        ePreInitArray           = 0x10,
        eGroup                  = 0x11,
        eExtendedSectionIndices = 0x12,
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

    enum class AuxiliaryVectorType
    {
        eAtNull         = 0,
        eAtIgnore       = 1,
        eAtExecFd       = 2,
        eAtPhdr         = 3,
        eAtPhEnt        = 4,
        eAtPhNum        = 5,
        eAtPageSz       = 6,
        eAtBase         = 7,
        eAtFlags        = 8,
        eAtEntry        = 9,
        eAtNotElf       = 10,
        eAtUid          = 11,
        eAtEUid         = 12,
        eAtGid          = 13,
        eAtEGid         = 14,
        eAtPlatform     = 15,
        eAtHwCap        = 16,
        eAtClkTck       = 17,
        eAtSecure       = 23,
        eAtBasePlatform = 24,
        eAtRandom       = 25,
        eAtHwCap2       = 26,
        eAtExecFn       = 31,
        eAtExeBase      = 32,
        eAtExeSize      = 33,
    };

    struct AuxiliaryVector
    {
        AuxiliaryVectorType Type;
        uintptr_t           EntryPoint;
        uintptr_t           ProgramHeadersAddress;
        usize               ProgramHeaderEntrySize;
        usize               ProgramHeaderCount;
    };
    class Image
    {
      public:
        bool LoadFromMemory(u8* data, usize size);
        bool ResolveSymbols(Span<Sym*> symbolTable);

        bool Load(PathView path, PageMap* pageMap, AddressSpace& addressSpace,
                  uintptr_t loadBase = 0);
        bool Load(Pointer image, usize size);

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

        inline const Vector<Symbol>& GetSymbols() const { return m_Symbols; }
        inline StringView            GetLdPath() const { return m_LdPath; }

      private:
        Buffer                                  m_Image;

        Header                                  m_Header;
        Vector<ProgramHeader>                   m_ProgramHeaders;
        Vector<SectionHeader>                   m_Sections;
        AuxiliaryVector                         m_AuxiliaryVector;

        SectionHeader*                          m_SymbolSection = nullptr;
        SectionHeader*                          m_StringSection = nullptr;
        CTOS_UNUSED u8*                         m_StringTable   = nullptr;

        Vector<Symbol>                          m_Symbols;
        std::unordered_map<StringView, Pointer> m_SymbolTable;
        StringView                              m_LdPath;

        bool                                    Parse();
        void                                    LoadSymbols();

        bool ParseProgramHeaders(ByteStream<Endian::eLittle>& stream);
        bool ParseSectionHeaders(ByteStream<Endian::eLittle>& stream);

        template <typename T>
        void Read(T* buffer, isize offset, isize count = sizeof(T))
        {
            std::memcpy(buffer, m_Image.Raw() + offset, count);
        }
    };
}; // namespace ELF
