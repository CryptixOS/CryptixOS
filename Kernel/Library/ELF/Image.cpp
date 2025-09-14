/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF/Image.hpp>
#include <Library/Module.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/PMM.hpp>
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Memory/ByteStream.hpp>
#include <Prism/String/StringUtils.hpp>
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
    ErrorOr<void> Image::LoadFromMemory(u8* data, usize size)
    {
        m_Image.Resize(size + 100);
        Memory::Copy(m_Image.Raw(), data, size);

        return Parse();
    }

    ErrorOr<void> Image::Load(FileDescriptor* file, Pointer loadBase)
    {
        if (!file) return Error(EBADF);
        auto inode = file->INode();

        if (!inode) return Error(ENOENT);
        return Load(inode, loadBase);
    }
    ErrorOr<void> Image::Load(::Ref<INode> inode, Pointer loadBase)
    {
        isize fileSize = inode->Size();
        m_Image.Resize(fileSize + 100);

        m_LoadBase = loadBase;
        if (inode->Read(m_Image.Raw(), 0, fileSize) != fileSize)
            return Error(EIO);
        if (!Parse()) return Error(ENOEXEC);

        return {};
    }

    usize Image::ProgramHeaderCount() { return m_Header.ProgramEntryCount; }
    const struct ProgramHeader& Image::ProgramHeader(usize index)
    {
        Pointer header = m_Image.Raw()
                       + (m_Header.ProgramHeaderTableOffset
                          + index * m_Header.ProgramEntrySize);

        return *header.As<struct ProgramHeader>();
    }
    usize Image::SectionHeaderCount() { return m_Header.SectionEntryCount; }
    const struct SectionHeader& Image::SectionHeader(usize index)
    {
        Pointer header = m_Image.Raw()
                       + (m_Header.SectionHeaderTableOffset
                          + index * m_Header.SectionEntrySize);

        return *header.As<struct SectionHeader>();
    }

    void Image::ForEachProgramHeader(ProgramHeaderIterator it)
    {
        usize entryCount = m_Header.ProgramEntryCount;
        for (usize i = 0; i < entryCount; i++)
            if (it(ProgramHeader(i)) == IterationResult::eBreak) break;
    }
    void Image::ForEachSectionHeader(SectionHeaderIterator& it)
    {
        usize entryCount = m_Header.SectionEntryCount;
        for (usize i = 0; i < entryCount; i++)
            if (it(SectionHeader(i)) == IterationResult::eBreak) break;
    }
    void Image::ForEachRelocationEntry(RelocationEntryIterator it)
    {
        for (usize i = 0; i < SectionHeaderCount(); ++i)
        {
            const auto& relocSection = SectionHeader(i);
            if (relocSection.Type != ToUnderlying(SectionType::eRelA)) continue;

            // Target section that relocations apply to
            u32 targetIndex = relocSection.Info;
            if (targetIndex >= SectionHeaderCount())
            {
                LogWarn(
                    "ELF: Invalid relocation section index => {}, section "
                    "header count: {}",
                    targetIndex, SectionHeaderCount());
                continue;
            }

            auto* relocs = reinterpret_cast<RelocationEntry*>(
                m_Image.Raw() + relocSection.Offset);
            usize relocCount = relocSection.Size / sizeof(RelocationEntry);

            for (usize r = 0; r < relocCount; ++r)
            {
                const auto& reloc = relocs[r];
                if (it(relocSection, reloc) == IterationResult::eBreak)
                    goto end;
            }
        }
    end:
    }

    void Image::ForEachSymbolEntry(SymbolEntryIterator it)
    {
        if (m_SymbolSection->Size == 0) return;
        if (m_StringSection->Size == 0) return;

        Symbol* symbolTable = reinterpret_cast<Symbol*>(
            m_Image.Raw() + m_SymbolSection->Offset);

        usize count = m_SymbolSection->Size / m_SymbolSection->EntrySize;
        for (usize i = 0; i < count; i++)
        {
            auto name = LookupString(symbolTable[i].Name);
            if (it(symbolTable[i], name) == IterationResult::eBreak) break;
        }
    }
    void Image::ForEachSymbol(SymbolIterator it)
    {
        if (m_Symbols.IsEmpty() && !LoadSymbols()) return;

        for (const auto& [name, value] : m_Symbols)
            if (it(name, value) == IterationResult::eBreak) break;
    }

    Pointer Image::LookupSymbol(StringView symbol) const
    {
        auto it = m_Symbols.Find(symbol);
        if (it == m_Symbols.end()) return nullptr;

        return it->Value;
    }
    void Image::DumpSymbols()
    {
        SymbolEntryIterator it;

        usize               i = 0;
        it.BindLambda(
            [&](Symbol& symbol, StringView name) -> IterationResult
            {
                u64  value        = symbol.Value;
                auto sectionIndex = symbol.SectionIndex;

                if (!name.Empty())
                    LogInfo("ELF Raw Symbol[{}]: '{}' => `{}`", i, name,
                            sectionIndex != ToUnderlying(SectionType::eNull)
                                ? fmt::format("{:#x}", value)
                                : "Undefined");

                ++i;
                return IterationResult::eContinue;
            });
        ForEachSymbolEntry(it);
    }

    ErrorOr<void> Image::Parse()
    {
        ByteStream<Endian::eNative> stream(m_Image.Raw(), m_Image.Size());

        stream >> m_Header;
        auto signature
            = StringView(reinterpret_cast<char*>(&m_Header.Magic), 4);

        if (signature != ELF::MAGIC)
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

        if (m_Header.InstructionSet != InstructionSet::eAMDx86_64
            && m_Header.InstructionSet != InstructionSet::eArm64)
        {
            LogError(
                "ELF: Only x86_64 and AArch64 instruction sets are supported");
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
            switch (current.Type)
            {
                case HeaderType::eProgramHeader:
                    m_AuxiliaryVector.ProgramHeaderAddress
                        = current.VirtualAddress + m_LoadBase.Raw();
                    break;
                case HeaderType::eInterp:
                {
                    char* path = new char[current.SegmentSizeInFile + 1];
                    Read(path, current.Offset, current.SegmentSizeInFile);
                    path[current.SegmentSizeInFile] = 0;

                    m_InterpreterPath
                        = StringView(path, current.SegmentSizeInFile);
                    break;
                }

                default: break;
            }

            auto aligned = Math::AlignDown(
                Pointer(m_Image.Raw()).Offset(current.VirtualAddress),
                PMM::PAGE_SIZE);
            auto size = Math::AlignUp(
                current.SegmentSizeInMemory
                    + Pointer(m_Image.Raw()).Offset(current.VirtualAddress)
                    - aligned,
                PMM::PAGE_SIZE);

            VMM::GetKernelPageMap()->ProtectRange(aligned, size,
                                                  PageAttributes::eRWX);
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
        if (!m_StringTable || index >= m_StringSection->Size) return ""_sv;
        return &m_StringTable[index];
    }

    bool Image::ParseSectionHeaders()
    {
        for (usize i = 0; i < m_Header.SectionEntryCount; i++)
        {
            const auto& shdr = SectionHeader(i);
            auto        type = static_cast<SectionType>(shdr.Type);

            if (type == SectionType::eSymbolTable)
                m_SymbolSection = const_cast<struct SectionHeader*>(&shdr);
            else if (type == SectionType::eStringTable)
            {
                m_StringSection = const_cast<struct SectionHeader*>(&shdr);
                m_StringTable   = reinterpret_cast<const char*>(m_Image.Raw()
                                                                + shdr.Offset);
            }
#if 1
            auto sectionName = LookupString(shdr.Name);

            if (!sectionName.Empty()
                && (sectionName.Compare(0, 4, ".got"_sv) == 0))
            {
                LogDebug("ELF: Found .got section, {}", sectionName);
                m_GotSection = const_cast<struct SectionHeader*>(&shdr);
            }
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

            if (symbolTable[i].SectionIndex == ToUnderlying(SectionType::eNull)
                || name.Empty())
                continue;
            m_Symbols[name] = addr;
        }

        return true;
    }
} // namespace ELF
