/*
 * Created by v1tr10l7 on 12.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ELF/Loader.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/VMM.hpp>
#include <VFS/VFS.hpp>

namespace ELF
{
    Loader::Loader()
        : m_Image(CreateRef<ELF::Image>())
    {
    }

    ErrorOr<void> Loader::LoadImage(PathView path)
    {
        auto pathRes
            = TryOrRet(VFS::ResolvePath(VFS::RootDirectoryEntry().Raw(), path));
        auto dentry = pathRes.Entry;

        return LoadImage(dentry);
    }
    ErrorOr<void> Loader::LoadImage(Ref<DirectoryEntry> dentry)
    {
        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);

        return LoadImage(inode);
    }
    ErrorOr<void> Loader::LoadImage(INode* inode)
    {
        auto status = m_Image->Load(inode);

        if (!status) return Error(status.Error());
        return {};
    }
    ErrorOr<void> Loader::LoadImage(Ref<FileDescriptor> file)
    {
        auto status = m_Image->Load(file.Raw());

        if (!status) return Error(status.Error());
        return {};
    }
    ErrorOr<void> Loader::LoadImage(u8* data, usize size)
    {
        auto status = m_Image->LoadFromMemory(data, size);

        if (!status) return Error(status.Error());
        return {};
    }
    ErrorOr<void> Loader::LoadImage(Ref<ELF::Image> image)
    {
        m_Image = image;

        return {};
    }

    ErrorOr<void> Loader::LoadSegments(PageMap&       pageMap,
                                       AddressSpace&  addressSpace,
                                       PageAttributes flags)
    {
        const auto& header    = m_Image->Header();

        Pointer     minVirt   = ~u64{0};
        Pointer     maxVirt   = 0;

        usize       totalSize = 0;
        for (usize i = 0; i < header.ProgramEntryCount; i++)
        {
            const auto& segment = m_Image->ProgramHeader(i);
            if (segment.Type != HeaderType::eLoad) continue;

            auto alignment = Max(PMM::PAGE_SIZE, segment.Alignment);

            auto start
                = Math::AlignDown(segment.VirtualAddress, PMM::PAGE_SIZE);
            auto end = Math::AlignUp(segment.VirtualAddress
                                         + segment.SegmentSizeInMemory,
                                     alignment);
            minVirt  = Min(minVirt.Raw(), start);
            maxVirt  = Max(maxVirt.Raw(), end);

            LogDebug(
                "ELF::Loader: ProgramHeader[{}] => \n"
                "start => {:#x}, end => {:#x}\n"
                "end - start => {:#x}",
                i, start, end, end - start);
            totalSize += end - start;
        }
        LogDebug("ELF::Loader: Total Size => {:#x}", totalSize);

        m_Size           = maxVirt.Raw() - minVirt.Raw();
        m_Size           = totalSize;
        // m_Size *= 2;

        auto alignedSize = Math::AlignUp(m_Size, PMM::PAGE_SIZE);
        m_LoadBase       = 0;
        m_PageMap        = &pageMap;
        m_AddressSpace   = &addressSpace;

        if (m_Image->Type() == ObjectType::eShared)
        {
            m_LoadBase      = VMM::AllocateSpace(alignedSize);
            m_Bias          = m_LoadBase.Raw() - minVirt.Raw();

            usize pageCount = Math::DivRoundUp(alignedSize, PMM::PAGE_SIZE);
            auto  phys      = PMM::CallocatePages(pageCount);

            LogTrace(
                "ELF::Loader: Mapping {:#x} bytes at {:#x} to {:#x}, mapping "
                "end => {:#x}",
                alignedSize, phys, m_LoadBase.Raw(),
                m_LoadBase.Offset(alignedSize));
            pageMap.MapRange(m_LoadBase, phys, alignedSize,
                             PageAttributes::eRWX | flags);

            auto region = CreateRef<Region>(phys, m_LoadBase, alignedSize);
            region->SetAccessMode(VMM::Access::eRead | VMM::Access::eWrite
                                  | VMM::Access::eExecute);
            addressSpace.Insert(m_LoadBase, region);
        }

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
            u8*   targetBase    = m_LoadBase.Offset<u8*>(targetSection.Address);

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
            usize loadStart = m_LoadBase;
            usize loadEnd   = m_LoadBase.Offset(m_Size);
            if (Pointer(patch) < loadStart
                || Pointer(patch).Offset(sizeof(u64)) > loadEnd)
            {
                status = EFAULT;
                LogWarn("ELF: Relocation patch at {:#x} out of bounds",
                        reinterpret_cast<u64>(patch));
                return IterationResult::eBreak;
            }

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
            const u64 segmentVirtBias = m_Bias.Offset(segment.VirtualAddress);

            if (m_Image->Type() == ObjectType::eExecutable)
            {
                usize misalign  = segment.VirtualAddress & (PMM::PAGE_SIZE - 1);
                usize pageCount = Math::DivRoundUp(
                    segment.SegmentSizeInMemory + misalign, PMM::PAGE_SIZE);

                Pointer phys = PMM::CallocatePages(pageCount);
                Assert(phys);

                auto  virt = segment.VirtualAddress + m_LoadBase.Raw();
                usize size = pageCount * PMM::PAGE_SIZE;
                Assert(m_PageMap->MapRange(virt, phys, size,
                                           PageAttributes::eRWXU
                                               | PageAttributes::eWriteBack));
                auto region = new Region(
                    phys, segment.VirtualAddress + m_LoadBase.Raw(), size);
                using VMM::Access;
                region->SetAccessMode(Access::eReadWriteExecute
                                      | Access::eUser);

                m_AddressSpace->Insert(region->VirtualBase(), region);
                Memory::Copy(phys.Offset<Pointer>(misalign).ToHigherHalf(),
                             m_Image->Raw().Offset(segment.Offset),
                             segment.SegmentSizeInFile);
            }
            else
            {
                Pointer headerStart = segmentVirtBias;
                auto headerEnd = headerStart.Offset(segment.SegmentSizeInFile);
                Assert(headerEnd <= m_LoadBase.Offset(m_Size));

                LogDebug(
                    "ELF::Loader: Loading segment at offset {:#x} of size "
                    "{:#x} bytes at address {:#x}",
                    segment.Offset, segment.SegmentSizeInFile,
                    headerStart.Raw());
                Memory::Copy(headerStart, m_Image->Raw().Offset(segment.Offset),
                             segment.SegmentSizeInFile);

                if (segment.SegmentSizeInMemory > segment.SegmentSizeInFile)
                    Memory::Fill(headerStart.Offset(segment.SegmentSizeInFile),
                                 0,
                                 segment.SegmentSizeInMemory
                                     - segment.SegmentSizeInFile);
            }
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
                    m_InitArray = m_LoadBase.Offset(
                        entry.Data.Address
                        + (m_Bias ? (m_Bias.Raw() - m_LoadBase.Raw()) : 0));
                    m_InitArray = reinterpret_cast<u8*>(entry.Data.Address
                                                        + m_Bias.Raw());
                    break;
                case DynamicEntryType::eFiniArray:
                    m_FiniArray = reinterpret_cast<u8*>(entry.Data.Address
                                                        + m_Bias.Raw());
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
