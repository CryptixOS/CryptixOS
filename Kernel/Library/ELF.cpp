/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF.hpp>
#include <Library/Module.hpp>

#include <Memory/PMM.hpp>
#include <Prism/Math.hpp>

#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <cstring>
#include <magic_enum/magic_enum.hpp>

#if 1
    #define ElfDebugLog(...) LogDebug(__VA_ARGS__)
#else
    #define ElfDebugLog(...)
#endif

namespace ELF
{
    bool Image::Load(std::string_view path, PageMap* pageMap,
                     std::vector<VMM::Region>& addressSpace, uintptr_t loadBase)
    {
        INode* file = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
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
                    addressSpace.push_back(
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
                && std::strncmp(MODULE_SECTION,
                               stringTable
                                + section->Name, std::strlen(MODULE_SECTION))
                       == 0)
            {
                std::string_view modName = stringTable + section->Name + strlen(MODULE_SECTION) + 1;

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

            m_ProgramHeaders.push_back(current);
        }

        m_Sections.clear();
        SectionHeader* sections = reinterpret_cast<SectionHeader*>(
            m_Image + m_Header.SectionHeaderTableOffset);
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)

        {
            m_Sections.push_back(sections[i]);

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
        Assert(m_SymbolSectionIndex.has_value());
        Assert(m_StringSectionIndex.has_value());
        if (m_SymbolSectionIndex.value() > m_Sections.size()) return;

        auto& section = m_Sections[m_SymbolSectionIndex.value()];
        if (section.Size <= 0) return;

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
            m_Symbols.push_back(sym);
        }

        std::sort(m_Symbols.begin(), m_Symbols.end(), std::less<Symbol>());
    }
} // namespace ELF
