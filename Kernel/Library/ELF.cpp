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
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <utility>

#if 0
    #define ElfDebugLog(...) LogDebug(__VA_ARGS__)
#else
    #define ElfDebugLog(...)
#endif

namespace ELF
{
    bool Image::LoadFromMemory(u8* data, usize size)
    {
        m_Image.Resize(size + 100);
        std::memcpy(m_Image.Raw(), data, size);

        ByteStream<Endian::eNative> stream(data, size);
        LogTrace("ELF: Kernel Executable Address -> '{:#x}'",
                 Pointer(data).Raw());

        stream >> m_Header;
        if (std::strncmp(reinterpret_cast<char*>(&m_Header.Magic), ELF::MAGIC,
                         4)
            != 0)
        {
            LogError("ELF: Invalid magic");
            return false;
        }
        if (m_Header.Bitness != Bitness::e64Bit)
        {
            LogError("ELF: Only 64-bit programs are supported");
            return false;
        }
        if (m_Header.Endianness != Endianness::eLittle)
        {
            LogError("ELF: BigEndian programs are not supported!");
            return false;
        }
        if (m_Header.HeaderVersion != CURRENT_ELF_HEADER_VERSION)
        {
            LogError("ELF: Invalid header version");
            return false;
        }
        if (m_Header.Abi != ABI::eSystemV)
        {
            LogError(
                "ELF: Header contains invalid abi ID, only SysV abi is "
                "supported");
            return false;
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
            return false;
        }
        if (m_Header.ElfVersion != CURRENT_ELF_HEADER_VERSION)
        {
            LogError("ELF: Invalid ELF version");
            return false;
        }

        if (!ParseProgramHeaders(stream) || !ParseSectionHeaders(stream))
            return false;

        if (m_SymbolSection && m_StringSection) LoadSymbols();

        return true;
    }
    bool Image::ResolveSymbols(Span<Sym*> symTab)
    {
        constexpr usize SHN_UNDEF    = 0;
        constexpr usize SHN_LOPROC   = 0xff00;

        auto            lookupSymbol = [&symTab](char* name) -> u64
        {
            (void)symTab;
            return 0;
        };

        for (const auto& section : m_Sections)
        {
            auto type = static_cast<SectionType>(section.Type);
            if (type != SectionType::eSymbolTable) continue;

            auto stringTableHeader
                = Pointer(m_Image.Raw() + m_Header.SectionHeaderTableOffset
                          + m_Header.SectionEntrySize * section.Link)
                      .As<SectionHeader>();
            char* symbolNames
                = reinterpret_cast<char*>(stringTableHeader->Address);
            Sym* symbolTable = reinterpret_cast<Sym*>(section.Address);

            for (usize sym = 0; sym < section.Size / sizeof(Sym); ++sym)
            {

                if (symbolTable[sym].SectionIndex > 0
                    && symbolTable[sym].SectionIndex < SHN_LOPROC)
                {
                    SectionHeader* shdr = Pointer(
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

        return true;
    }

    bool Image::Load(PathView path, PageMap* pageMap,
                     AddressSpace& addressSpace, uintptr_t loadBase)
    {
        DirectoryEntry* entry
            = VFS::ResolvePath(VFS::GetRootDirectoryEntry(), path)
                  .value()
                  .Entry;
        if (!entry) return false;

        auto file = entry->INode();
        if (!file) return_err(false, ENOENT);

        isize fileSize = file->Stats().st_size;
        // m_Image        = new u8[fileSize];
        m_Image.Resize(fileSize + 100);

        if (file->Read(m_Image.Raw(), 0, fileSize) != fileSize || !Parse())
            return false;

        LoadSymbols();
        for (auto& sym : m_Symbols) sym.Address += loadBase;

        for (const auto& current : m_ProgramHeaders)
        {
            switch (current.Type)
            {
                case HeaderType::eLoad:
                {
                    usize misalign
                        = current.VirtualAddress & (PMM::PAGE_SIZE - 1);
                    usize pageCount = Math::DivRoundUp(
                        current.SegmentSizeInMemory + misalign, PMM::PAGE_SIZE);

                    uintptr_t phys = PMM::CallocatePages<uintptr_t>(pageCount);
                    Assert(phys);

                    usize size = pageCount * PMM::PAGE_SIZE;

                    Assert(pageMap->MapRange(
                        current.VirtualAddress + loadBase, phys, size,
                        PageAttributes::eRWXU | PageAttributes::eWriteBack));

                    auto region = new Region(
                        phys, current.VirtualAddress + loadBase, size);

                    using VirtualMemoryManager::Access;
                    region->SetProt(Access::eReadWriteExecute | Access::eUser,
                                    PROT_READ | PROT_WRITE | PROT_EXEC);

                    addressSpace.Insert(region->GetVirtualBase(), region);

                    Read(ToHigherHalfAddress<void*>(phys + misalign),
                         current.Offset, current.SegmentSizeInFile);
                    ElfDebugLog(
                        "Virtual Address: {:#x}, sizeInFile: {}, "
                        "sizeInMemory: "
                        "{}",
                        current.VirtualAddress + loadBase,
                        current.SegmentSizeInFile, current.SegmentSizeInMemory);
                    break;
                }
                case HeaderType::eProgramHeader:
                    m_AuxiliaryVector.ProgramHeadersAddress
                        = current.VirtualAddress + loadBase;
                    break;
                case HeaderType::eInterp:
                {
                    char* path = new char[current.SegmentSizeInFile + 1];
                    Read(path, current.Offset, current.SegmentSizeInFile);
                    path[current.SegmentSizeInFile] = 0;

                    m_LdPath = StringView(path, current.SegmentSizeInFile);
                    break;
                }

                default: break;
            }
        }

        m_AuxiliaryVector.Type       = AuxiliaryVectorType::eAtBase;
        m_AuxiliaryVector.EntryPoint = m_Header.EntryPoint + loadBase;
        m_AuxiliaryVector.ProgramHeaderEntrySize = m_Header.ProgramEntrySize;
        m_AuxiliaryVector.ProgramHeaderCount     = m_Header.ProgramEntryCount;
        ElfDebugLog("EntryPoint: {:#x}", m_AuxiliaryVector.EntryPoint);
        return true;
    }
    bool Image::Load(Pointer image, usize size)
    {
        m_Image.Resize(size * 2);
        std::memcpy(m_Image.Raw(), image.As<u8>(), size);
        if (!Parse()) return false;

        if (m_StringSection && m_SymbolSection) LoadSymbols();
        return true;
    }

    bool Image::LoadModules(const u64 sectionCount, SectionHeader* sections,
                            char* stringTable)
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

    bool Image::Parse()
    {
        Read(&m_Header, 0);

        ElfDebugLog("ELF: Verifying signature...");
        if (std::memcmp(&m_Header.Magic, MAGIC, 4) != 0)
        {
            LogError("ELF: Invalid magic");
            return false;
        }

        ElfDebugLog("ELF: Parsing program headers...");
        ProgramHeader current;
        for (usize i = 0; i < m_Header.ProgramEntryCount; i++)
        {
            isize headerOffset = m_Header.ProgramHeaderTableOffset
                               + i * m_Header.ProgramEntrySize;
            Read(&current, headerOffset);
            ElfDebugLog("ELF: Header[{}] = {}({})", i,
                        static_cast<usize>(current.Type),
                        magic_enum::enum_name(current.Type));

            m_ProgramHeaders.PushBack(current);
        }

        m_Sections.Clear();
        SectionHeader* sections = reinterpret_cast<SectionHeader*>(
            m_Image.Raw() + m_Header.SectionHeaderTableOffset);
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)

        {
            m_Sections.PushBack(sections[i]);

            if (sections[i].Type
                == ToUnderlying(SectionType::eSymbolTable))
            {
                ElfDebugLog("ELF: Found symbol section at: {}", i);
                m_SymbolSection = sections + i;
            }
            else if (sections[i].Type
                         == ToUnderlying(SectionType::eStringTable)
                     && i != m_Header.SectionNamesIndex)

            {
                ElfDebugLog("ELF: Found string section at: {}", i);
                m_StringSection = sections + i;
            }
        }

        return true;
    }
    void Image::LoadSymbols()
    {
        ElfDebugLog("ELF: Loading symbols...");
        if (m_SymbolSection->Size == 0 || m_SymbolSection->EntrySize == 0)
            return;
        if (m_StringSection->Size == 0 || m_StringSection->EntrySize == 0)
            return;

        char* strtab
            = reinterpret_cast<char*>(m_Image.Raw() + m_StringSection->Offset);
        const Sym* symtab
            = reinterpret_cast<Sym*>(m_Image.Raw() + m_SymbolSection->Offset);

        usize entryCount = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < entryCount; i++)
        {
            auto    name = StringView(&strtab[symtab[i].Name]);
            Pointer addr = symtab[i].Value;

            if (symtab[i].SectionIndex == 0x00 || name.Empty()) continue;
            LogDebug("ELF: Loaded symbol: '{}' => '{}'", name, addr);
            m_SymbolTable[name] = addr;
        }
    }

    bool Image::ParseProgramHeaders(ByteStream<Endian::eLittle>& stream)
    {
        m_ProgramHeaders.Reserve(m_Header.ProgramEntryCount);
        for (usize i = 0; i < m_Header.ProgramEntryCount; i++)
        {
            ProgramHeader& phdr         = m_ProgramHeaders.EmplaceBack();
            isize          headerOffset = m_Header.ProgramHeaderTableOffset
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
            SectionHeader& shdr     = m_Sections.EmplaceBack();
            u64 sectionHeaderOffset = m_Header.SectionHeaderTableOffset
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
