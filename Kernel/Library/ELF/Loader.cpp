/*
 * Created by v1tr10l7 on 12.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF/Loader.hpp>

#include <Memory/VMM.hpp>

namespace ELF
{
    ErrorOr<void> Loader::LoadSegments()
    {
        const auto& header = m_Image->Header();
        for (usize i = 0; i < header.ProgramEntryCount; i++)
        {
            const auto& programHeader = m_Image->ProgramHeader(i);
            if (programHeader.SegmentSizeInFile == 0) continue;
            if (programHeader.Type != HeaderType::eLoad
                && programHeader.Type != HeaderType::eDynamic)
                continue;

            u64 top = programHeader.VirtualAddress
                    + programHeader.SegmentSizeInMemory;
            top = ((top - 1) / programHeader.Alignment + 1)
                * programHeader.Alignment;
            m_Size = Max(m_Size, top);
        }
        m_Size *= 2;

        auto pageMap     = VMM::GetKernelPageMap();
        auto alignedSize = Math::AlignUp(m_Size, PMM::PAGE_SIZE);
        m_LoadBase       = VMM::AllocateSpace(alignedSize);

        usize pageCount  = Math::DivRoundUp(m_Size, PMM::PAGE_SIZE);
        auto  phys       = PMM::CallocatePages(pageCount);
        // TODO(v1tr10l7): take pagemap and address space as arguments, and
        // potentially abstract away both into one class
        pageMap->MapRange(m_LoadBase, phys, alignedSize, PageAttributes::eRWX);

        Image::ProgramHeaderIterator it;
        it.Bind<&Loader::LoadSegment>(this);
        m_Image->ForEachProgramHeader(it);
        return {};
    }
    ErrorOr<void> Loader::ResolveSymbols(SymbolLookup lookup)
    {
        auto resolveSymbol
            = [this, &lookup](const auto& section,
                              const auto& reloc) -> IterationResult
        {
            const auto& symbolTableSection
                = m_Image->SectionHeader(section.Link);
            const auto& stringTableSection
                = m_Image->SectionHeader(symbolTableSection.Link);

            auto* stringTable = reinterpret_cast<const char*>(
                m_Image->Raw().Offset(stringTableSection.Offset));
            auto* symbols = reinterpret_cast<Symbol*>(
                m_Image->Raw().Offset(symbolTableSection.Offset));

            u32         symbolIndex   = reloc.Info >> 32;
            auto&       symbol        = symbols[symbolIndex];

            const char* symbolName    = stringTable + symbol.Name;
            u64         symbolAddress = 0;
            if (symbol.SectionIndex == ToUnderlying(SectionType::eNull))
                symbolAddress = lookup(symbolName);
            else symbolAddress = m_LoadBase.Offset(symbol.Value);

            m_Symbols[symbolName] = symbolAddress;
            return IterationResult::eContinue;
        };

        Image::RelocationEntryIterator resolveIt(resolveSymbol);
        m_Image->ForEachRelocationEntry(resolveIt);
        return {};
    }
    ErrorOr<void> Loader::ApplyRelocations()
    {
        ErrorCode status = no_error;

        auto      applyRelocation
            = [this,
               &status](const struct SectionHeader&   section,
                        const struct RelocationEntry& reloc) -> IterationResult
        {
            u32   targetIndex   = section.Info;
            auto& targetSection = m_Image->SectionHeader(targetIndex);
            u8*   targetBase    = m_LoadBase.Offset<u8*>(targetSection.Offset);

            auto  type = static_cast<RelocationType>(reloc.Info & 0xffffffff);
            u32   symbolIndex = reloc.Info >> 32;
            u8*   patch       = targetBase + reloc.Offset;

            // Symbol table and string table
            const auto& symbolTableSection
                = m_Image->SectionHeader(section.Link);
            const auto& stringTableSection
                = m_Image->SectionHeader(symbolTableSection.Link);

            auto  imageRaw    = m_Image->Raw();
            auto* stringTable = reinterpret_cast<const char*>(
                imageRaw.Offset(stringTableSection.Offset));

            auto* symbols = reinterpret_cast<Symbol*>(
                imageRaw.Offset(symbolTableSection.Offset));

            auto&       symbol        = symbols[symbolIndex];
            const char* symbolName    = stringTable + symbol.Name;

            auto        symbolIt      = m_Symbols.Find(symbolName);
            u64         symbolAddress = 0;
            if (symbolIt != m_Symbols.end()) symbolAddress = symbolIt->Value;
            usize loadEnd = m_LoadBase.Offset(m_Size);

            switch (type)
            {
                case RelocationType::e64:
                case RelocationType::eGlobDat:
                case RelocationType::eJumpSlot:
                {
                    *reinterpret_cast<u64*>(patch) = symbolAddress;
                    break;
                }
                case RelocationType::eRelative:
                {
                    symbolAddress = m_LoadBase.Offset(reloc.Addend);
                    *reinterpret_cast<u64*>(patch) = symbolAddress;
                    m_Symbols[symbolName]          = symbolAddress;

                    if (symbolAddress < m_LoadBase.Raw()
                        || symbolAddress >= loadEnd)
                    {
                        status = EFAULT;
                        LogWarn(
                            "ELF: The address at `{:#x}` is out of "
                            "bounds",
                            symbolAddress);
                    }
                    break;
                }
                default:
                    LogError("ELF: Unsupported relocation type: {}",
                             static_cast<u32>(type));
                    status = ENOEXEC;
                    break;
            }

            if (symbolName == "ModuleInit"_sv) m_EntryPoint = symbolAddress;
            return IterationResult::eContinue;
        };

        Image::RelocationEntryIterator relocIt;
        relocIt.BindLambda(applyRelocation);
        m_Image->ForEachRelocationEntry(relocIt);

        if (status) return Error(status);

        m_Image->LoadSymbols(m_Symbols);
        return {};
    }

    Pointer Loader::LookupSymbol(StringView name) const
    {
        auto found = m_Symbols.Find(name);
        if (found != m_Symbols.end()) return found->Value;

        return nullptr;
    }
    void Loader::ForEachSymbol(SymbolIterator it)
    {
        for (const auto& [name, value] : m_Symbols)
            if (it(name, value) == IterationResult::eBreak) break;
    }
    IterationResult Loader::LoadSegment(const ProgramHeader& segment)
    {
        if (segment.Type == HeaderType::eLoad
            || segment.Type == HeaderType::eDynamic)
        {
            auto headerStart = m_LoadBase.Offset(segment.VirtualAddress);
            auto headerEnd   = headerStart + segment.SegmentSizeInFile;
            Assert(headerEnd <= m_LoadBase.Offset(m_Size));

            Memory::Copy(headerStart, m_Image->Raw().Offset(segment.Offset),
                         segment.SegmentSizeInFile);

            if (segment.SegmentSizeInMemory > segment.SegmentSizeInFile)
                Memory::Fill(headerStart + segment.SegmentSizeInFile, 0,
                             segment.SegmentSizeInMemory
                                 - segment.SegmentSizeInFile);
        }

        if (segment.Type != HeaderType::eDynamic)
            return IterationResult::eContinue;

        const auto dynamicTable = reinterpret_cast<DynamicEntry*>(
            m_Image->Raw().Offset(segment.Offset));
        for (usize i = 0; i < segment.SegmentSizeInFile / sizeof(DynamicEntry);
             i++)
        {
            const auto& entry = dynamicTable[i];
            switch (entry.Tag)
            {
                case DynamicEntryType::eInitArray:
                    m_InitArray = m_LoadBase.Offset(entry.Data.Address);
                    break;
                case DynamicEntryType::eFiniArray:
                    m_FiniArray = m_LoadBase.Offset(entry.Data.Address);
                    break;
                case DynamicEntryType::eInitArraySize:
                    m_InitArraySize = entry.Data.Value;
                    break;
                case DynamicEntryType::eFiniArraySize:
                    m_FiniArraySize = entry.Data.Value;
                    break;

                default: break;
            };
        }

        return IterationResult::eContinue;
    }
}; // namespace ELF
