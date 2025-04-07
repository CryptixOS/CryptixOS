/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF.hpp>
#include <Library/Module.hpp>

#include <Memory/PMM.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Math.hpp>

#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <algorithm>
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

        return true;
    }
    bool Image::Load(PathView path, PageMap* pageMap,
                     Vector<VMM::Region>& addressSpace, uintptr_t loadBase)
    {
        INode* file
            = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path, true));
        if (!file) return_err(false, ENOENT);

        isize fileSize = file->GetStats().st_size;
        m_Image        = new u8[fileSize];

        if (file->Read(m_Image, 0, fileSize) != fileSize || !Parse())
            return false;

        LoadSymbols();
        for (auto& sym : m_Symbols) sym.address += loadBase;

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
                    addressSpace.PushBack(
                        {phys, current.VirtualAddress + loadBase, size});

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

                    m_LdPath
                        = std::string_view(path, current.SegmentSizeInFile);
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
        m_Image = image.As<u8>();
        if (!Parse()) return false;

        LoadSymbols();
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
                std::string_view modName
                    = stringTable + section->Name + strlen(MODULE_SECTION) + 1;

                LogInfo("Mod: {}", stringTable + section->Name);
                LogInfo("Mod: {}", modName);
                for (auto offset = section->Address;
                     offset < section->Address + section->Size;
                     offset += sizeof(Module))
                {
                    auto module = reinterpret_cast<Module*>(offset);
                    auto ret    = LoadModule(module);
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
            m_Image + m_Header.SectionHeaderTableOffset);
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)

        {
            m_Sections.PushBack(sections[i]);

            if (sections[i].Type
                == std::to_underlying(SectionType::eSymbolTable))
            {
                ElfDebugLog("ELF: Found symbol section at: {}", i);
                m_SymbolSectionIndex = i;
            }
            else if (sections[i].Type
                         == std::to_underlying(SectionType::eStringTable)
                     && i != m_Header.SectionNamesIndex)

            {
                ElfDebugLog("ELF: Found string section at: {}", i);
                m_StringSectionIndex = i;
            }
        }

        // stringSectionIndex = header.sectionNamesIndex;
        return true;
    }
    void Image::LoadSymbols()
    {
        ElfDebugLog("ELF: Loading symbols...");
        if (!m_SymbolSectionIndex.has_value()) return;
        // Assert(m_SymbolSectionIndex.has_value());
        if (!m_StringSectionIndex.has_value()) return;
        if (m_SymbolSectionIndex.value() >= m_Sections.Size()) return;

        auto& section = m_Sections[m_SymbolSectionIndex.value()];
        if (section.Size <= 0 || section.EntrySize == 0) return;

        const Sym* symtab = reinterpret_cast<Sym*>(m_Image + section.Offset);
        m_StringTable     = reinterpret_cast<u8*>(m_Image + section.Offset);
        char* strtab      = reinterpret_cast<char*>(m_StringTable);

        usize entryCount  = section.Size / section.EntrySize;
        for (usize i = 0; i < entryCount; i++)
        {
            auto name = std::string_view(&strtab[symtab[i].Name]);
            if (symtab[i].SectionIndex == 0x00 || name.empty()) continue;

            uintptr_t addr = symtab[i].Value;
            Symbol    sym;
            sym.name    = const_cast<char*>(name.data());
            sym.address = addr;
            m_Symbols.PushBack(sym);
        }

        std::sort(m_Symbols.begin(), m_Symbols.end(), std::less<Symbol>());
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
                    LogTrace("ELF: Program Header[{}] -> Unused Entry", i);
                    break;
                case HeaderType::eLoad:
                    LogTrace("ELF: Program Header[{}] -> Loadable Segment", i);
                    break;
                case HeaderType::eDynamic:
                    LogTrace(
                        "ELF: Program Header[{}] -> Dynamic Linking "
                        "Information",
                        i);
                    break;
                case HeaderType::eInterp:
                    LogTrace(
                        "ELF: Program Header[{}] -> Interpreter Information",
                        i);
                    break;
                case HeaderType::eNote:
                    LogTrace("ELF: Program Header[{}] -> Auxiliary Information",
                             i);
                    break;
                case HeaderType::eProgramHeader:
                    LogTrace("ELF: Program Header[{}] -> Program Header", i);
                    break;
                case HeaderType::eTLS:
                    LogTrace(
                        "ELF: Program Header[{}] -> Thread-Local Storage "
                        "template",
                        i);
                    break;

                default:
                    LogWarn(
                        "ELF: Unrecognized program header type at index '{}' "
                        "-> {}",
                        i, std::to_underlying(phdr.Type));
                    break;
            }
        }
        return true;
    }
    bool Image::ParseSectionHeaders(ByteStream<Endian::eLittle>& stream)
    {
        m_Sections.Reserve(m_Header.SectionEntryCount);

        auto sectionTypeToString = [](SectionType type) -> const char*
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

            const char* sectionName = stringTable + shdr.Name;
            LogTrace("ELF: Section[{}] -> '{}': {}", i, sectionName,
                     sectionTypeToString(static_cast<SectionType>(shdr.Type)));
        }

        return true;
    }
} // namespace ELF
