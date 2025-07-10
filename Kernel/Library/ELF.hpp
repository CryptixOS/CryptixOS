/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/Span.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>

#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>

class AddressSpace;
class PageMap;
class FileDescriptor;
class INode;

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

    enum class RelocationType : u32
    {
        eNone      = 0,  // No relocation
        e64        = 1,  // Direct 64-bit: *(u64*)patch = S + A
        ePC32      = 2,  // PC-relative 32-bit signed: *(i32*)patch = S + A - P
        eGOT32     = 3,  // GOT entry 32-bit
        ePLT32     = 4,  // PLT entry 32-bit
        eCopy      = 5,  // Runtime copy (dynamic linking)
        eGlobDat   = 6,  // Set GOT entry to symbol address
        eJumpSlot  = 7,  // Same as GlobDat
        eRelative  = 8,  // Relative to base: *(u64*)patch = B + A
        eGotOffset = 9,  // GOT offset
        eGotPC     = 10, // PC-relative GOT offset
        e32        = 10, // Direct 32-bit: *(u32*)patch = S + A
        e32S       = 11, // Direct signed 32-bit
        e16        = 12, // Direct 16-bit
        ePC16      = 13, // PC-relative 16-bit
        e8         = 14, // Direct 8-bit
        ePC8       = 15, // PC-relative 8-bit
        eDTPMod64  = 16, // TLS
        eDTPOff64  = 17, // TLS
        eTPOff64   = 18, // TLS
        eTLSDESC   = 19, // TLS descriptor
        eTLSDESC_CALL = 20, // TLS call
        eIRELATIVE    = 37, // IFUNC-style relative relocation (PLT)
    };
    struct [[gnu::packed]] RelocationEntry
    {
        u64 Offset;
        u64 Info;
        i64 Addend;
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
    struct [[gnu::packed]] Symbol
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
        Pointer             EntryPoint;
        Pointer             ProgramHeaderAddress;
        usize               ProgramHeaderEntrySize;
        usize               ProgramHeaderCount;
    };
    class Image : public RefCounted
    {
      public:
        ErrorOr<void> LoadFromMemory(u8* data, usize size);
        ErrorOr<void> Load(FileDescriptor* file, Pointer loadBase = 0);
        ErrorOr<void> Load(INode* inode, Pointer loadBase = 0);

        Pointer       Raw() const { return m_Image.Raw(); }
        inline const struct Header& Header() const { return m_Header; }

        usize                       ProgramHeaderCount();
        struct ProgramHeader*       ProgramHeader(usize index);
        usize                       SectionHeaderCount();
        struct SectionHeader*       SectionHeader(usize index);

        using ProgramHeaderEnumerator = Delegate<bool(struct ProgramHeader*)>;
        void ForEachProgramHeader(ProgramHeaderEnumerator enumerator);

        using SectionHeaderEnumerator = Delegate<bool(struct SectionHeader*)>;
        void ForEachSectionHeader(SectionHeaderEnumerator enumerator);

        using SymbolEntryEnumerator
            = Delegate<bool(Symbol& symbol, StringView name)>;
        void ForEachSymbolEntry(SymbolEntryEnumerator enumerator);
        using SymbolEnumerator = Delegate<bool(StringView name, Pointer value)>;
        void ForEachSymbol(SymbolEnumerator enumerator);

        using SymbolLookup = Delegate<u64(StringView name)>;
        ErrorOr<void>  ApplyRelocations(SymbolLookup lookup);

        ErrorOr<void>  ResolveSymbols(SymbolLookup lookup);
        ErrorOr<void>  ResolveSymbols(struct SectionHeader& section,
                                      SymbolLookup          lookup);
        Pointer        LookupSymbol(StringView name);
        void           DumpSymbols();

        StringView     LookupString(usize index);

        inline Pointer EntryPoint() const
        {
            return m_AuxiliaryVector.EntryPoint;
        }
        inline Pointer ProgramHeaderAddress() const
        {
            return m_AuxiliaryVector.ProgramHeaderAddress;
        }
        inline usize ProgramHeaderEntrySize() const
        {
            return m_AuxiliaryVector.ProgramHeaderEntrySize;
        }

        inline PathView InterpreterPath() const { return m_InterpreterPath; }

      private:
        Buffer                        m_Image;
        Pointer                       m_LoadBase = nullptr;

        struct Header                 m_Header;
        AuxiliaryVector               m_AuxiliaryVector;

        struct SectionHeader*         m_SymbolSection = nullptr;
        struct SectionHeader*         m_StringSection = nullptr;
        const char*                   m_StringTable   = nullptr;

        RedBlackTree<StringView, u64> m_Symbols;
        StringView                    m_InterpreterPath;

        ErrorOr<void>                 Parse();
        bool                          ParseSectionHeaders();
        bool                          LoadSymbols();

        template <typename T>
        void Read(T* buffer, isize offset, isize count = sizeof(T))
        {
            Memory::Copy(buffer, m_Image.Raw() + offset, count);
        }
    };
}; // namespace ELF
