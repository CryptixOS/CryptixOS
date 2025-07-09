/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF.hpp>
#include <Library/Module.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/PMM.hpp>
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/Math.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <cstring>

#if 0
    #define ElfDebugLog(...) LogDebug(__VA_ARGS__)
#else
    #define ElfDebugLog(...)
#endif

namespace ELF
{
    ErrorOr<void> Image::LoadFromMemory(u8* data, usize size)
    {
        m_Image.Resize(size + 100);
        Memory::Copy(m_Image.Raw(), data, size);

        auto status = Parse();
        RetOnError(status);

        return {};
        Pointer loadAddress = VMM::AllocateSpace(size);
        for (usize i = 0; i < m_Header.SectionEntryCount; ++i)
        {
            struct SectionHeader* sectionHeader
                = reinterpret_cast<struct SectionHeader*>(
                    loadAddress.Raw() + m_Image.Raw()
                    + m_Header.SectionHeaderTableOffset
                    + m_Header.SectionEntrySize * i);
            if (sectionHeader->Type == ToUnderlying(SectionType::eNoBits))
            {
                sectionHeader->Address
                    = VMM::AllocateSpace(sectionHeader->Size);
                continue;
            }

            sectionHeader->Address = loadAddress.Raw() + sectionHeader->Offset;
            if (sectionHeader->Alignment
                && (sectionHeader->Address & (sectionHeader->Alignment - 1)))
                LogWarn(
                    "ELF: Module probably is not aligned correctly: %#zx %ld\n",
                    sectionHeader->Address, sectionHeader->Alignment);
        }

        for (usize i = 0; i < m_Header.SectionEntryCount; ++i)
        {
            struct SectionHeader* sectionHeader
                = reinterpret_cast<struct SectionHeader*>(
                    loadAddress.Raw() + m_Image.Raw()
                    + m_Header.SectionHeaderTableOffset
                    + m_Header.SectionEntrySize * i);
            if (sectionHeader->Type != ToUnderlying(SectionType::eSymbolTable))
                continue;

            struct SectionHeader* stringTable
                = reinterpret_cast<struct SectionHeader*>(
                    loadAddress.Raw() + m_Header.SectionHeaderTableOffset
                    + m_Header.SectionEntrySize * sectionHeader->Link);
            CTOS_UNUSED char* symbols
                = reinterpret_cast<char*>(stringTable->Address);
            Sym* symbolTable = reinterpret_cast<Sym*>(sectionHeader->Address);

            for (usize symbolIndex = 0;
                 symbolIndex < sectionHeader->Size / sizeof(Sym); ++symbolIndex)
            {
                if (symbolTable[symbolIndex].SectionIndex > 0
                    && usize(symbolTable[symbolIndex].SectionIndex)
                           < ToUnderlying(HeaderType::eProcessorSpecificStart))
                {
                    struct SectionHeader* header
                        = reinterpret_cast<struct SectionHeader*>(
                            loadAddress.Raw()
                            + m_Header.SectionHeaderTableOffset
                            + m_Header.SectionEntrySize
                                  * symbolTable[symbolIndex].SectionIndex);

                    symbolTable[symbolIndex].Value
                        = symbolTable[symbolIndex].Value + header->Address;
                    continue;
                }
                else if (symbolTable[symbolIndex].Value
                         == ToUnderlying(HeaderType::eNone))
                    ; // TODO(v1tr10l7): Lookup symbols
            }
        }

        for (unsigned int i = 0; i < m_Header.SectionEntryCount; ++i)
        {
            struct SectionHeader* sectionHeader
                = (struct SectionHeader*)(loadAddress.Raw()
                                          + m_Header.SectionHeaderTableOffset
                                          + m_Header.SectionEntrySize * i);
            if (sectionHeader->Type != ToUnderlying(SectionType::eRelA))
                continue;

            RelocationEntry* table = (RelocationEntry*)sectionHeader->Address;

            // Get the section these relocations apply to //
            struct SectionHeader* targetSection
                = (struct SectionHeader*)(loadAddress.Raw()
                                          + m_Header.SectionHeaderTableOffset
                                          + m_Header.SectionEntrySize
                                                * sectionHeader->Info);

            // Get the symbol table //
            struct SectionHeader* symbolSection
                = (struct SectionHeader*)(loadAddress.Raw()
                                          + m_Header.SectionHeaderTableOffset
                                          + m_Header.SectionEntrySize
                                                * sectionHeader->Link);
            Sym* symbolTable = (Sym*)symbolSection->Address;

#define S               (symbolTable[ELF64_R_SYM(table[rela].Info)].Value)
#define A               (table[rela].Addend)
#define T32             (*(uint32_t*)target)
#define T64             (*(uint64_t*)target)
#define P               (target)

#define ELF64_R_TYPE(i) ((i) & 0xffffffff)
#define ELF64_R_SYM(i)  ((i) >> 32)
#define R_X86_64_64     1  /* Direct 64 bit  */
#define R_X86_64_32     10 /* Direct 32 bit zero extended */
#define R_X86_64_PC32   2  /* PC relative 32 bit signed */

            for (unsigned int rela = 0;
                 rela < sectionHeader->Size / sizeof(RelocationEntry); ++rela)
            {
                uintptr_t target = table[rela].Offset + targetSection->Address;
                switch (ELF64_R_TYPE(table[rela].Info))
                {
                    case R_X86_64_64: T64 = S + A; break;
                    case R_X86_64_32: T32 = S + A; break;
                    case R_X86_64_PC32: T32 = S + A - P; break;
                    default:
                        AssertFmt(false,
                                  "Module: Unsupported relocation {} found\n",
                                  ELF64_R_TYPE(table[rela].Info));
                }
            }
        }

        return {};
    }

    ErrorOr<void> Image::Load(FileDescriptor* file, Pointer loadBase)
    {
        if (!file) return Error(EBADF);
        auto inode = file->INode();

        if (!inode) return Error(ENOENT);
        return Load(inode, loadBase);
    }
    ErrorOr<void> Image::Load(INode* inode, Pointer loadBase)
    {
        isize fileSize = inode->Stats().st_size;
        m_Image.Resize(fileSize + 100);

        m_LoadBase = loadBase;
        if (inode->Read(m_Image.Raw(), 0, fileSize) != fileSize)
            return Error(EIO);
        if (!Parse()) return Error(ENOEXEC);

        return {};
    }

    usize Image::ProgramHeaderCount() { return m_Header.ProgramEntryCount; }
    struct ProgramHeader* Image::ProgramHeader(usize index)
    {
        Pointer header = m_Image.Raw()
                       + (m_Header.ProgramHeaderTableOffset
                          + index * m_Header.ProgramEntrySize);

        return header.As<struct ProgramHeader>();
    }
    usize Image::SectionHeaderCount() { return m_Header.SectionEntryCount; }
    struct SectionHeader* Image::SectionHeader(usize index)
    {
        Pointer header = m_Image.Raw()
                       + (m_Header.SectionHeaderTableOffset
                          + index * m_Header.SectionEntrySize);

        return header.As<struct SectionHeader>();
    }

    void Image::ForEachProgramHeader(ProgramHeaderEnumerator enumerator)
    {
        usize entryCount = m_Header.ProgramEntryCount;
        for (usize i = 0; i < entryCount; i++)
            if (!enumerator(ProgramHeader(i))) break;
    }
    void Image::ForEachSectionHeader(SectionHeaderEnumerator enumerator)
    {
        usize entryCount = m_Header.SectionEntryCount;
        for (usize i = 0; i < entryCount; i++)
            if (!enumerator(SectionHeader(i))) break;
    }

    void Image::ForEachSymbol(SymbolEnumerator enumerator)
    {
        if (!m_SymbolSection || !m_StringSection) return;

        char* stringTable
            = reinterpret_cast<char*>(m_Image.Raw() + m_StringSection->Offset);
        Sym* symbolTable
            = reinterpret_cast<Sym*>(m_Image.Raw() + m_SymbolSection->Offset);

        usize count = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < count; i++)
        {
            StringView name = &stringTable[symbolTable[i].Name];
            if (!enumerator(symbolTable[i], name)) break;
        }
    }

    ErrorOr<void> Image::ResolveSymbols(Span<Sym*> symbolTable)
    {
        constexpr usize SHN_UNDEF    = 0;
        constexpr usize SHN_LOPROC   = 0xff00;

        auto            lookupSymbol = [&symbolTable](char* name) -> u64
        {
            (void)symbolTable;
            return 0;
        };

        for (const auto& section : m_Sections)
        {
            auto type = static_cast<SectionType>(section.Type);
            if (type != SectionType::eSymbolTable) continue;

            auto stringTableHeader
                = Pointer(m_Image.Raw() + m_Header.SectionHeaderTableOffset
                          + m_Header.SectionEntrySize * section.Link)
                      .As<struct SectionHeader>();
            char* symbolNames
                = reinterpret_cast<char*>(stringTableHeader->Address);
            Sym* symbolTable = reinterpret_cast<Sym*>(section.Address);

            for (usize sym = 0; sym < section.Size / sizeof(Sym); ++sym)
            {

                if (symbolTable[sym].SectionIndex > 0
                    && symbolTable[sym].SectionIndex < SHN_LOPROC)
                {
                    struct SectionHeader* shdr = Pointer(
                        m_Image.Raw() + m_Header.SectionHeaderTableOffset
                        + m_Header.SectionEntrySize
                              * symbolTable[sym].SectionIndex);
                    symbolTable[sym].Value += shdr->Address;
                }
                else if (symbolTable[sym].SectionIndex == SHN_UNDEF)
                    symbolTable[sym].Value
                        = lookupSymbol(symbolNames + symbolTable[sym].Name);

                // TODO(v1tr10l7): module data
                if (symbolTable[sym].Name
                    && !strcmp(symbolNames + symbolTable[sym].Name, "metadata"))
                    ;
            }
        }

        return {};
    }
    Pointer Image::LookupSymbol(StringView symbolName)
    {
        SymbolEnumerator it;
        Pointer          found = nullptr;

        it.BindLambda(
            [&](Sym& symbol, StringView name) -> bool
            {
                u64 value = symbol.Value;
                if (name == symbolName)
                {
                    found = value;
                    return false;
                }

                return true;
            });
        ForEachSymbol(it);

        return found;
    }
    void Image::DumpSymbols()
    {
        SymbolEnumerator it;

        usize            i = 0;
        it.BindLambda(
            [&](Sym& symbol, StringView name) -> bool
            {
                u64             value        = symbol.Value;
                auto            sectionIndex = symbol.SectionIndex;
                constexpr usize SHN_UNDEF    = 0;

                if (!name.Empty())
                    LogInfo("ELF Raw Symbol[{}]: '{}' => `{}`", i, name,
                            sectionIndex != SHN_UNDEF
                                ? fmt::format("{:#x}", value)
                                : "Undefined");

                ++i;
                return true;
            });
        ForEachSymbol(it);
    }

    bool Image::LoadModules(const u64             sectionCount,
                            struct SectionHeader* sections, char* stringTable)
    {
        bool found = false;
        for (usize i = 0; i < sectionCount; i++)
        {
            auto section = &sections[i];
            if (section->Size != 0 && section->Size >= sizeof(Module)
                && std::strncmp(MODULE_SECTION, stringTable + section->Name,
                                std::strlen(MODULE_SECTION))
                       == 0)
            {
                [[maybe_unused]] StringView modName
                    = stringTable + section->Name + strlen(MODULE_SECTION) + 1;

                for (auto offset = section->Address;
                     offset < section->Address + section->Size;
                     offset += sizeof(Module))
                {
                    auto module = reinterpret_cast<Module*>(offset);
                    LogTrace("ELF: Loading module: {{ .Address: {:#x} }}",
                             (uintptr_t)module);

                    auto ret = LoadModule(module);
                    if (!found) found = ret;
                }
                break;
            }
        }

        return found;
    }

    ErrorOr<void> Image::Parse()
    {
        ByteStream<Endian::eNative> stream(m_Image.Raw(), m_Image.Size());
        LogTrace("ELF: Kernel Executable Address -> '{:#x}'",
                 Pointer(m_Image.Raw()).Raw());

        stream >> m_Header;
        if (std::strncmp(reinterpret_cast<char*>(&m_Header.Magic), ELF::MAGIC,
                         4)
            != 0)
        {
            LogError("ELF: Invalid magic");
            return Error(ENOEXEC);
        }
        if (m_Header.Bitness != Bitness::e64Bit)
        {
            LogError("ELF: Only 64-bit programs are supported");
            return Error(ENOEXEC);
        }
        if (m_Header.Endianness != Endianness::eLittle)
        {
            LogError("ELF: BigEndian programs are not supported!");
            return Error(ENOEXEC);
        }
        if (m_Header.HeaderVersion != CURRENT_ELF_HEADER_VERSION)
        {
            LogError("ELF: Invalid header version");
            return Error(ENOEXEC);
        }
        if (m_Header.Abi != ABI::eSystemV)
        {
            LogError(
                "ELF: Header contains invalid abi ID, only SysV abi is "
                "supported");
            return Error(ENOEXEC);
        }

        constexpr auto EXECUTABLE_TYPE_STRINGS = ToArray({
            "Relocatable",
            "Executable",
            "Shared",
            "Core",
        });
        LogTrace("ELF: Executable type -> '{}'",
                 m_Header.Type < 4 ? EXECUTABLE_TYPE_STRINGS[m_Header.Type]
                                   : "Unknown");
        if (m_Header.InstructionSet != InstructionSet::eAMDx86_64)
        {
            LogError("ELF: Only x86_64 instruction set is supported");
            return Error(ENOEXEC);
        }
        if (m_Header.ElfVersion != CURRENT_ELF_HEADER_VERSION)
        {
            LogError("ELF: Invalid ELF version");
            return Error(ENOEXEC);
        }

        if (!ParseProgramHeaders(stream) || !ParseSectionHeaders(stream))
            return Error(ENOEXEC);

        if (m_SymbolSection && m_StringSection) LoadSymbols();

        for (usize i = 0; i < m_Header.ProgramEntryCount; i++)
        {
            auto current = ProgramHeader(i);
            switch (current->Type)
            {
                case HeaderType::eProgramHeader:
                    m_AuxiliaryVector.ProgramHeadersAddress
                        = current->VirtualAddress + m_LoadBase.Raw();
                    break;
                case HeaderType::eInterp:
                {
                    char* path = new char[current->SegmentSizeInFile + 1];
                    Read(path, current->Offset, current->SegmentSizeInFile);
                    path[current->SegmentSizeInFile] = 0;

                    m_LdPath = StringView(path, current->SegmentSizeInFile);
                    break;
                }

                default: break;
            }
        }

        m_AuxiliaryVector.Type       = AuxiliaryVectorType::eAtBase;
        m_AuxiliaryVector.EntryPoint = m_Header.EntryPoint + m_LoadBase.Raw();
        m_AuxiliaryVector.ProgramHeaderEntrySize = m_Header.ProgramEntrySize;
        m_AuxiliaryVector.ProgramHeaderCount     = m_Header.ProgramEntryCount;
        ElfDebugLog("EntryPoint: {:#x}", m_AuxiliaryVector.EntryPoint);

        return {};
    }

    void Image::LoadSymbols()
    {
        ElfDebugLog("ELF: Loading symbols...");
        if (m_SymbolSection->Size == 0 || m_SymbolSection->EntrySize == 0)
            return;
        if (m_StringSection->Size == 0 || m_StringSection->EntrySize == 0)
            return;

        char* stringTable
            = reinterpret_cast<char*>(m_Image.Raw() + m_StringSection->Offset);
        const Sym* symbolTable
            = reinterpret_cast<Sym*>(m_Image.Raw() + m_SymbolSection->Offset);

        usize entryCount = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < entryCount; i++)
        {
            auto    name = StringView(&stringTable[symbolTable[i].Name]);
            Pointer addr = symbolTable[i].Value;

            if (symbolTable[i].SectionIndex == 0x00 || name.Empty()) continue;
            LogDebug("ELF: Loaded symbol: '{}' => '{}'", name, addr);
            m_SymbolTable[name] = addr;
        }
    }

    bool Image::ParseProgramHeaders(ByteStream<Endian::eLittle>& stream)
    {
        m_ProgramHeaders.Reserve(m_Header.ProgramEntryCount);
        for (usize i = 0; i < m_Header.ProgramEntryCount; i++)
        {
            struct ProgramHeader& phdr = m_ProgramHeaders.EmplaceBack();
            isize headerOffset         = m_Header.ProgramHeaderTableOffset
                               + i * m_Header.ProgramEntrySize;

            stream.Seek(headerOffset);
            stream >> phdr;

            switch (phdr.Type)
            {
                case HeaderType::eNone:
                    ElfDebugLog("ELF: Program Header[{}] -> Unused Entry", i);
                    break;
                case HeaderType::eLoad:
                    ElfDebugLog("ELF: Program Header[{}] -> Loadable Segment",
                                i);
                    break;
                case HeaderType::eDynamic:
                    ElfDebugLog(
                        "ELF: Program Header[{}] -> Dynamic Linking "
                        "Information",
                        i);
                    break;
                case HeaderType::eInterp:
                    ElfDebugLog(
                        "ELF: Program Header[{}] -> Interpreter Information",
                        i);
                    break;
                case HeaderType::eNote:
                    ElfDebugLog(
                        "ELF: Program Header[{}] -> Auxiliary Information", i);
                    break;
                case HeaderType::eProgramHeader:
                    ElfDebugLog("ELF: Program Header[{}] -> Program Header", i);
                    break;
                case HeaderType::eTLS:
                    ElfDebugLog(
                        "ELF: Program Header[{}] -> Thread-Local Storage "
                        "template",
                        i);
                    break;

                default:
                    LogWarn(
                        "ELF: Unrecognized program header type at index '{}' "
                        "-> {}",
                        i, ToUnderlying(phdr.Type));
                    break;
            }
        }
        return true;
    }
    bool Image::ParseSectionHeaders(ByteStream<Endian::eLittle>& stream)
    {
        m_Sections.Reserve(m_Header.SectionEntryCount);

        CTOS_UNUSED auto sectionTypeToString
            = [](SectionType type) -> const char*
        {
            switch (type)
            {
                case SectionType::eNull:
                    return "Unused section header table entry";
                case SectionType::eProgBits: return "Program Data";
                case SectionType::eSymbolTable: return "Symbol Table";
                case SectionType::eStringTable: return "String Table";
                case SectionType::eRelA:
                    return "Relocation entries with addends";
                case SectionType::eHash: return "Symbol Hash Table";
                case SectionType::eDynamic:
                    return "Dynamic Linking Information";
                case SectionType::eNote: return "Notes";
                case SectionType::eNoBits:
                    return "Program Space with no data(.bss)";
                case SectionType::eRel:
                    return "Relocation entries (no addends)";
                case SectionType::eDynSym: return "Dynamic Linker Symbol Table";
                case SectionType::eInitArray: return "Array of Constructors";
                case SectionType::eFiniArray: return "Array of Destructors";
                case SectionType::ePreInitArray:
                    return "Array of Pre-constructors";
                case SectionType::eGroup: return "Section Group";
                case SectionType::eExtendedSectionIndices:
                    return "Extended Section Indices";

                default: break;
            }
            return "Unrecognized";
        };

        auto sections = reinterpret_cast<ELF::SectionHeader*>(
            reinterpret_cast<u64>(stream.Raw())
            + m_Header.SectionHeaderTableOffset);
        auto stringTable = reinterpret_cast<char*>(
            reinterpret_cast<u64>(stream.Raw())
            + sections[m_Header.SectionNamesIndex].Offset);
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)
        {
            struct SectionHeader& shdr = m_Sections.EmplaceBack();
            u64 sectionHeaderOffset    = m_Header.SectionHeaderTableOffset
                                    + i * m_Header.SectionEntrySize;
            stream.Seek(sectionHeaderOffset);
            stream >> shdr;
            auto type = static_cast<SectionType>(shdr.Type);

            if (type == SectionType::eSymbolTable) m_SymbolSection = &shdr;
            else if (type == SectionType::eStringTable) m_StringSection = &shdr;

            CTOS_UNUSED const char* sectionName = stringTable + shdr.Name;
            ElfDebugLog(
                "ELF: Section[{}] -> '{}': {}", i, sectionName,
                sectionTypeToString(static_cast<SectionType>(shdr.Type)));
        }

        return true;
    }
} // namespace ELF
