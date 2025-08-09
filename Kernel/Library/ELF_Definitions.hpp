/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/String/StringView.hpp>

namespace ELF
{
    constexpr StringView MAGIC = "\177ELF"_sv;
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
    enum class ObjectType : u16
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

    struct CTOS_PACKED Header
    {
        u32                 Magic;
        enum Bitness        Bitness;
        enum Endianness     Endianness;
        u8                  HeaderVersion;
        ABI                 Abi;
        u64                 Padding;
        ObjectType          Type;
        enum InstructionSet InstructionSet;
        u32                 ElfVersion;
        u64                 EntryPoint;
        u64                 ProgramHeaderTableOffset;
        u64                 SectionHeaderTableOffset;
        u32                 Flags;
        u16                 HeaderSize;
        u16                 ProgramEntrySize;
        u16                 ProgramEntryCount;
        u16                 SectionEntrySize;
        u16                 SectionEntryCount;
        u16                 SectionNamesIndex;
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
    enum class SegmentAttributes : u32
    {
        eExecutable = 1,
        eWriteable  = 2,
        eReadable   = 4,
    };
    struct CTOS_PACKED ProgramHeader
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

    struct CTOS_PACKED SectionHeader
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

    struct CTOS_PACKED Symbol
    {
        u32 Name;
        u8  Info;
        u8  Other;
        u16 SectionIndex;
        u64 Value;
        u64 Size;
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
    struct CTOS_PACKED RelocationEntry
    {
        u64 Offset;
        u64 Info;
        i64 Addend;
    };

    enum class DynamicEntryType : i64
    {
        eNull             = 0,  // DT_NULL
        eNeeded           = 1,  // DT_NEEDED
        ePltRelSize       = 2,  // DT_PLTRELSZ
        ePltGot           = 3,  // DT_PLTGOT
        eHash             = 4,  // DT_HASH
        eStrTab           = 5,  // DT_STRTAB
        eSymTab           = 6,  // DT_SYMTAB
        eRela             = 7,  // DT_RELA
        eRelaSize         = 8,  // DT_RELASZ
        eRelaEnt          = 9,  // DT_RELAENT
        eStrSize          = 10, // DT_STRSZ
        eSymEnt           = 11, // DT_SYMENT
        eInit             = 12, // DT_INIT
        eFini             = 13, // DT_FINI
        eSoName           = 14, // DT_SONAME
        eRPath            = 15, // DT_RPATH (deprecated)
        eSymbolic         = 16, // DT_SYMBOLIC
        eRel              = 17, // DT_REL
        eRelSize          = 18, // DT_RELSZ
        eRelEnt           = 19, // DT_RELENT
        ePltRel           = 20, // DT_PLTREL
        eDebug            = 21, // DT_DEBUG
        eTextRel          = 22, // DT_TEXTREL
        eJmpRel           = 23, // DT_JMPREL
        eBindNow          = 24, // DT_BIND_NOW
        eInitArray        = 25, // DT_INIT_ARRAY
        eFiniArray        = 26, // DT_FINI_ARRAY
        eInitArraySize    = 27, // DT_INIT_ARRAYSZ
        eFiniArraySize    = 28, // DT_FINI_ARRAYSZ
        eRunPath          = 29, // DT_RUNPATH
        eFlags            = 30, // DT_FLAGS
        eEncoding         = 32, // DT_ENCODING (unspecified meaning)
        ePreInitArray     = 32, // DT_PREINIT_ARRAY
        ePreInitArraySize = 33, // DT_PREINIT_ARRAYSZ
        eSymTabShIndex    = 34, // DT_SYMTAB_SHNDX
        eRelr             = 35, // DT_RELR
        eRelrSize         = 36, // DT_RELRSZ
        eRelrEnt          = 37, // DT_RELRENT

        // Values in OS-specific range
        eLoOs             = 0x6000000d, // DT_LOOS
        eHiOs             = 0x6ffff000, // DT_HIOS

        // Values in Processor-specific range
        eLoProc           = 0x70000000, // DT_LOPROC
        eHiProc           = 0x7fffffff  // DT_HIPROC
    };

    struct CTOS_PACKED DynamicEntry
    {
        DynamicEntryType Tag; /* Dynamic entry type */
        union
        {
            u64 Value;   /* Integer value */
            u64 Address; /* Address value */
        } Data;
    };

    enum class AuxiliaryValueType
    {
        eNull                   = 0,
        eIgnore                 = 1,
        eProgramFd              = 2,
        eProgramHeader          = 3,
        eProgramHeaderEntrySize = 4,
        eProgramHeaderCount     = 5,
        ePageSize               = 6,
        eInterpreterBase        = 7,
        eFlags                  = 8,
        eEntry                  = 9,
        eNotElf                 = 10,
        eUid                    = 11,
        eEUid                   = 12,
        eGid                    = 13,
        eEGid                   = 14,
        ePlatform               = 15,
        eHwCap                  = 16,
        eClkTck                 = 17,
        eSecure                 = 23,
        eBasePlatform           = 24,
        eRandom                 = 25,
        eHwCap2                 = 26,
        eExecFn                 = 31,
        eExeBase                = 32,
        eExeSize                = 33,
    };

    struct AuxiliaryVector
    {
        AuxiliaryValueType Type;
        Pointer            EntryPoint;
        Pointer            ProgramHeaderAddress;
        usize              ProgramHeaderEntrySize;
        usize              ProgramHeaderCount;
    };
}; // namespace ELF
