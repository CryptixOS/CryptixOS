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
#include <Prism/Memory/ByteStream.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Math.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#if 0
    #define ElfDebugLog(...) LogDebug(__VA_ARGS__)
#else
    #define ElfDebugLog(...)
#endif

namespace ELF
{
    constexpr usize SHN_UNDEF  = 0;
    constexpr usize SHN_LOPROC = 0xff00;

    ErrorOr<void>   Image::LoadFromMemory(u8* data, usize size)
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
            Symbol* symbolTable
                = reinterpret_cast<Symbol*>(sectionHeader->Address);

            for (usize symbolIndex = 0;
                 symbolIndex < sectionHeader->Size / sizeof(Symbol);
                 ++symbolIndex)
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
            Symbol* symbolTable = (Symbol*)symbolSection->Address;

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

    void Image::ForEachSymbolEntry(SymbolEntryEnumerator enumerator)
    {
        if (m_SymbolSection->Size == 0) return;
        if (m_StringSection->Size == 0) return;

        Symbol* symbolTable = reinterpret_cast<Symbol*>(
            m_Image.Raw() + m_SymbolSection->Offset);

        usize count = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < count; i++)
        {
            auto name = LookupString(symbolTable[i].Name);
            if (!enumerator(symbolTable[i], name)) break;
        }
    }
    void Image::ForEachSymbol(SymbolEnumerator enumerator)
    {
        if (m_Symbols.IsEmpty() && !LoadSymbols()) return;

        for (const auto& [name, value] : m_Symbols)
            if (!enumerator(name, value)) break;
    }

    ErrorOr<void> Image::ApplyRelocations(SymbolLookup lookup)
    {
        for (usize i = 0; i < SectionHeaderCount(); ++i)
        {
            auto& relocSection = *SectionHeader(i);
            if (relocSection.Type != ToUnderlying(SectionType::eRelA)) continue;

            // Target section that relocations apply to
            u32 targetIndex = relocSection.Info;
            if (targetIndex >= SectionHeaderCount()) return Error(ENOEXEC);

            auto&       targetSection = *SectionHeader(targetIndex);
            u8*         targetBase    = m_Image.Raw() + targetSection.Offset;

            // Symbol table and string table
            const auto& symtabSection = *SectionHeader(relocSection.Link);
            const auto& strtabSection = *SectionHeader(symtabSection.Link);

            auto*       symbols       = reinterpret_cast<Symbol*>(m_Image.Raw()
                                                                  + symtabSection.Offset);
            auto*       strtab        = reinterpret_cast<const char*>(
                m_Image.Raw() + strtabSection.Offset);
            usize symCount = symtabSection.Size / sizeof(Symbol);

            auto* relocs   = reinterpret_cast<RelocationEntry*>(
                m_Image.Raw() + relocSection.Offset);
            usize relocCount = relocSection.Size / sizeof(RelocationEntry);

            for (usize r = 0; r < relocCount; ++r)
            {
                const auto&    reloc = relocs[r];
                RelocationType type
                    = static_cast<RelocationType>(reloc.Info & 0xffffffff);
                u32 symIndex = reloc.Info >> 32;
                u8* patch    = targetBase + reloc.Offset;

                u64 symAddr  = 0;
                if (symIndex >= symCount) return Error(ENOEXEC);

                const auto& sym     = symbols[symIndex];
                const char* symName = strtab + sym.Name;

                if (sym.SectionIndex != SHN_UNDEF)
                {
                    if (sym.SectionIndex >= SectionHeaderCount())
                        return Error(ENOEXEC);

                    const auto& defSection = *SectionHeader(sym.SectionIndex);
                    symAddr                = reinterpret_cast<u64>(
                        m_Image.Raw() + defSection.Offset + sym.Value);
                }
                else
                {
                    symAddr = lookup(symName);
                    if (!symAddr)
                    {
                        LogError("ELF: Unresolved symbol: {}", symName);
                        return Error(ENOEXEC);
                    }
                }

                switch (type)
                {
                    case RelocationType::e64:
                    {
                        u64 value                      = symAddr + reloc.Addend;
                        *reinterpret_cast<u64*>(patch) = value;
                        break;
                    }
                    case RelocationType::ePC32:
                    {
                        i32 value
                            = static_cast<i32>(symAddr + reloc.Addend
                                               - reinterpret_cast<u64>(patch));
                        *reinterpret_cast<i32*>(patch) = value;
                        break;
                    }
                    case RelocationType::eRelative:
                    {
                        u64 value = reinterpret_cast<u64>(m_Image.Raw()
                                                          + reloc.Addend);
                        *reinterpret_cast<u64*>(patch) = value;
                        break;
                    }
                    default:
                        LogError("ELF: Unsupported relocation type: {}",
                                 static_cast<u32>(type));
                        return Error(ENOEXEC);
                }
            }
        }

        return {};
    }

    ErrorOr<void> Image::ResolveSymbols(SymbolLookup lookup)
    {
        for (usize i = 0; i < m_Header.SectionEntryCount; ++i)
        {
            struct SectionHeader& sectionHeader = *SectionHeader(i);

            if (sectionHeader.Type == ToUnderlying(SectionType::eNoBits))
            {
                usize pageCount
                    = Math::DivRoundUp(sectionHeader.Size, PMM::PAGE_SIZE);
                auto phys             = PMM::CallocatePages(pageCount);
                sectionHeader.Address = VMM::AllocateSpace(sectionHeader.Size);

                VMM::MapKernelRange(sectionHeader.Address, phys,
                                    sectionHeader.Size, PageAttributes::eRWXU);
            }
            else
            {
                sectionHeader.Address
                    = Pointer(m_Image.Raw() + sectionHeader.Offset).Raw();
                if (sectionHeader.Alignment
                    && (sectionHeader.Address & (sectionHeader.Alignment - 1)))
                    LogWarn(
                        "ELF: Section address is probably not aligned "
                        "correctly: {:#x} {}",
                        sectionHeader.Address, sectionHeader.Alignment);
            }
        }

        for (usize i = 0; i < SectionHeaderCount(); i++)
        {
            auto& section = *SectionHeader(i);
            auto  type    = static_cast<SectionType>(section.Type);
            if (type != SectionType::eSymbolTable) continue;

            auto status = ResolveSymbols(section, lookup);
            if (!status) LogError("ELF: Failed to resolve some symbols");
        }

        return {};
    }
    ErrorOr<void> Image::ResolveSymbols(struct SectionHeader& section,
                                        SymbolLookup          lookup)
    {
        auto stringTable
            = Pointer(m_Image.Raw() + m_Header.SectionHeaderTableOffset
                      + m_Header.SectionEntrySize * section.Link)
                  .As<struct SectionHeader>();
        char* strings
            = reinterpret_cast<char*>(stringTable->Offset + m_Image.Raw());
        Symbol* symbolTable
            = Pointer(m_Image.Raw() + section.Offset).As<Symbol>();

        usize symbolCount = section.Size / sizeof(Symbol);
        for (usize symbol = 0; symbol < symbolCount; ++symbol)
        {
            if (symbolTable[symbol].SectionIndex > 0
                && symbolTable[symbol].SectionIndex < SHN_LOPROC)
            {
                auto shdr = Pointer(m_Header.SectionHeaderTableOffset
                                    + m_Header.SectionEntrySize
                                          * symbolTable[symbol].SectionIndex
                                    + m_Image.Raw())
                                .As<struct SectionHeader>();
                symbolTable[symbol].Value += shdr->Address;
            }
            else if (symbolTable[symbol].SectionIndex == SHN_UNDEF)
                symbolTable[symbol].Value
                    = lookup(strings + symbolTable[symbol].Name);

            if (symbolTable[symbol].Name
                && !std::strcmp(strings + symbolTable[symbol].Name, "metadata"))
                LogTrace("ELF: Found module data => {:#x}",
                         symbolTable[symbol].Value);
        }

        return {};
    }

    Pointer Image::LookupSymbol(StringView symbol)
    {
        auto it = m_Symbols.Find(symbol);
        if (it == m_Symbols.end()) return nullptr;

        return it->Value;
    }
    void Image::DumpSymbols()
    {
        SymbolEntryEnumerator it;

        usize                 i = 0;
        it.BindLambda(
            [&](Symbol& symbol, StringView name) -> bool
            {
                u64  value        = symbol.Value;
                auto sectionIndex = symbol.SectionIndex;

                if (!name.Empty())
                    LogInfo("ELF Raw Symbol[{}]: '{}' => `{}`", i, name,
                            sectionIndex != SHN_UNDEF
                                ? fmt::format("{:#x}", value)
                                : "Undefined");

                ++i;
                return true;
            });
        ForEachSymbolEntry(it);
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

        if (!ParseSectionHeaders()) return Error(ENOEXEC);
        if (m_SymbolSection && m_StringSection) LoadSymbols();

        for (usize i = 0; i < m_Header.ProgramEntryCount; i++)
        {
            auto current = ProgramHeader(i);
            switch (current->Type)
            {
                case HeaderType::eProgramHeader:
                    m_AuxiliaryVector.ProgramHeaderAddress
                        = current->VirtualAddress + m_LoadBase.Raw();
                    break;
                case HeaderType::eInterp:
                {
                    char* path = new char[current->SegmentSizeInFile + 1];
                    Read(path, current->Offset, current->SegmentSizeInFile);
                    path[current->SegmentSizeInFile] = 0;

                    m_InterpreterPath
                        = StringView(path, current->SegmentSizeInFile);
                    break;
                }

                default: break;
            }
        }

        m_AuxiliaryVector.Type       = AuxiliaryValueType::eInterpreterBase;
        m_AuxiliaryVector.EntryPoint = m_Header.EntryPoint + m_LoadBase.Raw();
        m_AuxiliaryVector.ProgramHeaderEntrySize = m_Header.ProgramEntrySize;
        m_AuxiliaryVector.ProgramHeaderCount     = m_Header.ProgramEntryCount;
        ElfDebugLog("EntryPoint: {:#x}", m_AuxiliaryVector.EntryPoint);

        return {};
    }

    StringView Image::LookupString(usize index)
    {
        return &m_StringTable[index];
    }

    bool Image::ParseSectionHeaders()
    {
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)
        {
            struct SectionHeader& shdr = *SectionHeader(i);
            auto                  type = static_cast<SectionType>(shdr.Type);

            if (type == SectionType::eSymbolTable) m_SymbolSection = &shdr;
            else if (type == SectionType::eStringTable)
            {
                m_StringSection = &shdr;
                m_StringTable   = reinterpret_cast<const char*>(m_Image.Raw()
                                                                + shdr.Offset);
            }

#if 0
            auto stringTable = reinterpret_cast<char*>(
                m_Image.Raw()
                + SectionHeader(m_Header.SectionNamesIndex)->Offset);
            const char* sectionName = stringTable + shdr.Name;
            ElfDebugLog(
                "ELF: Section[{}] -> '{}': {}", i, sectionName,
                sectionTypeToString(static_cast<SectionType>(shdr.Type)));
#endif
        }

        return true;
    }
    bool Image::LoadSymbols()
    {
        if (m_SymbolSection->Size == 0) return false;
        if (m_StringSection->Size == 0) return false;

        ElfDebugLog("ELF: Loading symbols...");
        const Symbol* symbolTable = reinterpret_cast<Symbol*>(
            m_Image.Raw() + m_SymbolSection->Offset);

        usize entryCount = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < entryCount; i++)
        {
            auto    name = LookupString(symbolTable[i].Name);
            Pointer addr = symbolTable[i].Value;

            if (symbolTable[i].SectionIndex == SHN_UNDEF || name.Empty())
                continue;
            m_Symbols[name] = addr;
        }

        return true;
    }
} // namespace ELF
