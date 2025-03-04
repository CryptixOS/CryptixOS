/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "ELF.hpp"

#include "Memory/PMM.hpp"
#include "Prism/Math.hpp"

#include "VFS/INode.hpp"
#include "VFS/VFS.hpp"

#include <magic_enum/magic_enum.hpp>

#if 0
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
        image          = new u8[fileSize];

        if (file->Read(image, 0, fileSize) != fileSize || !Parse())
            return false;

        LoadSymbols();
        for (auto& sym : symbols) sym.address += loadBase;

        for (const auto& current : programHeaders)
        {
            switch (current.type)
            {
                case HeaderType::eLoad:
                {
                    usize misalign
                        = current.virtualAddress & (PMM::PAGE_SIZE - 1);
                    usize pageCount = Math::DivRoundUp(
                        current.segmentSizeInMemory + misalign, PMM::PAGE_SIZE);

                    uintptr_t phys = PMM::CallocatePages<uintptr_t>(pageCount);
                    Assert(phys);

                    usize size = pageCount * PMM::PAGE_SIZE;

                    Assert(pageMap->MapRange(
                        current.virtualAddress + loadBase, phys, size,
                        PageAttributes::eRWXU | PageAttributes::eWriteBack));
                    addressSpace.push_back(
                        {phys, current.virtualAddress + loadBase, size});

                    Read(ToHigherHalfAddress<void*>(phys + misalign),
                         current.offset, current.segmentSizeInFile);
                    ElfDebugLog(
                        "Virtual Address: {:#x}, sizeInFile: {}, sizeInMemory: "
                        "{}",
                        current.virtualAddress + loadBase,
                        current.segmentSizeInFile, current.segmentSizeInMemory);
                    break;
                }
                case HeaderType::eProgramHeader:
                    auxv.ProgramHeadersAddress
                        = current.virtualAddress + loadBase;
                    break;
                case HeaderType::eInterp:
                {
                    char* path = new char[current.segmentSizeInFile + 1];
                    Read(path, current.offset, current.segmentSizeInFile);
                    path[current.segmentSizeInFile] = 0;

                    ldPath = std::string_view(path, current.segmentSizeInFile);
                    break;
                }

                default: break;
            }
        }

        auxv.EntryPoint             = header.entryPoint + loadBase;
        auxv.ProgramHeaderEntrySize = header.programEntrySize;
        auxv.ProgramHeaderCount     = header.programEntryCount;
        ElfDebugLog("EntryPoint: {:#x}", auxv.EntryPoint);
        return true;
    }

    bool Image::Parse()
    {
        Read(&header, 0);

        ElfDebugLog("ELF: Verifying signature...");
        if (std::memcmp(&header.magic, MAGIC, 4) != 0)
        {
            LogError("ELF: Invalid magic");
            return false;
        }

        ElfDebugLog("ELF: Parsing program headers...");
        ProgramHeader current;
        for (usize i = 0; i < header.programEntryCount; i++)
        {
            isize headerOffset
                = header.programHeaderTableOffset + i * header.programEntrySize;
            Read(&current, headerOffset);
            ElfDebugLog("ELF: Header[{}] = {}({})", i,
                        static_cast<usize>(current.type),
                        magic_enum::enum_name(current.type));

            programHeaders.push_back(current);
        }

        sections.clear();
        SectionHeader* _sections = reinterpret_cast<SectionHeader*>(
            image + header.sectionHeaderTableOffset);
        for (usize i = 0; i < header.sectionEntryCount; i++)

        {
            sections.push_back(_sections[i]);

            if (_sections[i].type
                == std::to_underlying(SectionType::eSymbolTable))
            {
                ElfDebugLog("ELF: Found symbol section at: {}", i);
                symbolSectionIndex = i;
            }
            else if (_sections[i].type
                         == std::to_underlying(SectionType::eStringTable)
                     && i != header.sectionNamesIndex)

            {
                ElfDebugLog("ELF: Found string section at: {}", i);
                stringSectionIndex = i;
            }
        }

        // stringSectionIndex = header.sectionNamesIndex;
        return true;
    }
    void Image::LoadSymbols()
    {
        ElfDebugLog("ELF: Loading symbols...");
        Assert(symbolSectionIndex.has_value());
        Assert(stringSectionIndex.has_value());
        if (symbolSectionIndex.value() > sections.size()) return;

        auto& section = sections[symbolSectionIndex.value()];
        if (section.size <= 0) return;

        const Sym*  symtab = reinterpret_cast<Sym*>(image + section.offset);
        const char* stringTable
            = reinterpret_cast<const char*>(image + section.offset);

        usize entryCount = section.size / section.entrySize;
        for (usize i = 0; i < entryCount; i++)
        {
            auto name = std::string_view(&stringTable[symtab[i].name]);
            if (symtab[i].sectionIndex == 0x00 || name.empty()) continue;

            uintptr_t addr = symtab[i].value;
            Symbol    sym;
            sym.name    = const_cast<char*>(name.data());
            sym.address = addr;
            symbols.push_back(sym);
        }

        std::sort(symbols.begin(), symbols.end(), std::less<Symbol>());
    }
} // namespace ELF
