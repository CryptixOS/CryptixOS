/*
 * Created by v1tr10l7 on 12.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Debug.hpp>
#include <Library/ELF/Loader.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/VMM.hpp>
#include <VFS/VFS.hpp>

extern KeyValuePair<Pointer, usize> SignalTrampoline();

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
        if (!dentry) return Error(ENOENT);

        return LoadImage(dentry);
    }
    ErrorOr<void> Loader::LoadImage(Ref<DirectoryEntry> dentry)
    {
        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);

        return LoadImage(inode);
    }
    ErrorOr<void> Loader::LoadImage(Ref<INode> inode)
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

    ErrorOr<void> Loader::AllocateMemory(PageMap&       pageMap,
                                         AddressSpace&  addressSpace,
                                         PageAttributes flags, Pointer loadBase)
    {
        const auto& header = m_Image->Header();

        m_MinVirt          = ~u64{0};
        m_MaxVirt          = 0;

#if CTOS_ELF_DEBUG
        usize totalSize = 0;
#endif

        for (usize i = 0; i < header.ProgramEntryCount; i++)
        {
            const auto& segment = m_Image->ProgramHeader(i);
            if (segment.Type != HeaderType::eLoad) continue;

            auto alignment = Max(PMM::PAGE_SIZE, segment.Alignment);

            auto start
                = Math::AlignDown(segment.VirtualAddress, PMM::PAGE_SIZE);
            auto end  = Math::AlignUp(segment.VirtualAddress
                                          + segment.SegmentSizeInMemory,
                                      alignment);
            m_MinVirt = Min(m_MinVirt.Raw(), start);
            m_MaxVirt = Max(m_MaxVirt.Raw(), end);

#if CTOS_ELF_DEBUG
            LogDebug(
                "ELF::Loader: ProgramHeader[{}] => \n"
                "start => {:#x}, end => {:#x}\n"
                "end - start => {:#x}",
                i, start, end, end - start);
            totalSize += end - start;
#endif
        }

        auto [trampolinePhys, trampolineSize] = SignalTrampoline();
        trampolineSize          = Math::AlignUp(trampolineSize, PMM::PAGE_SIZE);

        m_Size                  = m_MaxVirt.Raw() - m_MinVirt.Raw();
        m_AlignedSize           = Math::AlignUp(m_Size, PMM::PAGE_SIZE);

        m_LoadBase              = 0;
        m_PageMap               = &pageMap;
        m_AddressSpace          = &addressSpace;

        Pointer virtBase        = nullptr;
        Pointer alignedVirtBase = nullptr;

        if (m_Image->Type() == ObjectType::eExecutable) loadBase = 0;
        else m_LoadBase = loadBase ?: VMM::AllocateSpace(m_AlignedSize * 2);
        m_Bias = m_Image->IsShared() ? m_LoadBase.Raw() - m_MinVirt.Raw() : 0;

#if CTOS_ELF_DEBUG
        LogDebug(
            "ELF::Loader: Loading executable =>\n"
            "minVirt => {:#x}, maxVirt => {:#x}\n"
            "totalSize => {:#x}, size => {:#x}, alignedSize => {:#x}\n"
            "loadBase => {:#x}, bias => {:#x}\n",
            m_MinVirt.Raw(), m_MaxVirt.Raw(), totalSize, m_Size, m_AlignedSize,
            m_LoadBase.Raw(), m_Bias.Raw());
#endif

        virtBase = m_Image->IsShared() ? m_LoadBase.Offset(m_MinVirt.Raw())
                                       : m_MinVirt.Raw();
        alignedVirtBase = Math::AlignDown(virtBase.Raw(), PMM::PAGE_SIZE);
        m_VirtMisalign  = virtBase.Raw() - alignedVirtBase.Raw();

        usize pageCount
            = Math::DivRoundUp(m_AlignedSize + m_VirtMisalign, PMM::PAGE_SIZE);
        m_Phys = PMM::CallocatePages(pageCount);
        if (!m_Phys)
        {
            LogError(
                "ELF::Loader: Failed to allocate {} pages for program headers",
                pageCount);
            return Error(ENOMEM);
        }

#if CTOS_ELF_DEBUG
        LogTrace(
            "ELF::Loader: Mapping {:#x} bytes at {:#x} to {:#x}, mapping "
            "end => {:#x}",
            m_AlignedSize, m_Phys, virtBase,
            alignedVirtBase.Offset(m_AlignedSize + m_VirtMisalign));
#endif

        usize totalSize = m_AlignedSize + m_VirtMisalign;
        if (!pageMap.MapRange(alignedVirtBase, m_Phys, totalSize, flags))
        {
            PMM::FreePages(m_Phys, pageCount);
            return Error(EFAULT);
        }

        m_TrampolineVirt = alignedVirtBase.Offset(totalSize);
        auto region      = CreateRef<Region>(m_Phys, alignedVirtBase,
                                             m_AlignedSize + m_VirtMisalign);
        region->SetAttributes(flags);
        using VMM::Access;
        region->SetAccessMode(Access::eReadWriteExecute | Access::eUser);
        addressSpace.Insert(region);

        auto trampolineFlags = PageAttributes::eRWXU;

        if (&addressSpace != VMM::GetKernelAddressSpace())
        {
            m_TrampolineVirt = Math::AlignUp(
                m_TrampolineVirt.Offset(PMM::PAGE_SIZE) * 32, PMM::PAGE_SIZE);
            auto trampolineRegion = CreateRef<Region>(
                trampolinePhys, m_TrampolineVirt, trampolineSize);
            trampolineRegion->SetAccessMode(Access::eReadWriteExecute
                                            | Access::eUser);
            // addressSpace.Insert(trampolineRegion);
            Assert(pageMap.MapRange(m_TrampolineVirt, trampolinePhys,
                                    trampolineSize, trampolineFlags));
        }

        return {};
    }
    ErrorOr<void> Loader::LoadSegments(PageMap&       pageMap,
                                       AddressSpace&  addressSpace,
                                       PageAttributes flags, Pointer loadBase)
    {
        RetOnError(AllocateMemory(pageMap, addressSpace, flags, loadBase));

#if CTOS_ELF_DEBUG
        Pointer virtBase = m_Image->IsShared()
                             ? m_LoadBase.Offset(m_MinVirt.Raw())
                             : m_MinVirt.Raw();

        // Debug: verify every page in the window maps to the expected physical
        // page
        for (usize offset = 0; offset < m_AlignedSize; offset += PMM::PAGE_SIZE)
        {
            Pointer virt         = virtBase.Offset(offset);
            Pointer expectedPhys = m_Phys.Offset(offset + m_VirtMisalign);

            Pointer physFromMap  = m_PageMap->Virt2Phys(virt);

            usize   offsetInPage = virt & (PMM::PAGE_SIZE - 1);
            Pointer actualPhys   = physFromMap & (PMM::PAGE_SIZE - 1)
                                     ? physFromMap
                                     : physFromMap.Offset<Pointer>(offsetInPage);

            if (actualPhys.Raw() != expectedPhys.Raw())
                LogWarn(
                    "ELF::Loader: mapping mismatch at page off {:#x}: expected "
                    "phys {:#x}, got {:#x}; virt {:#x}",
                    offset, expectedPhys, actualPhys, virt);
        }
#endif

        ErrorCode status = no_error;
        m_Image->ForEachProgramHeader(
            [this, &status](const ProgramHeader& segment) -> IterationResult
            {
                auto result = ParseSegment(segment);
                if (!result)
                {
                    status = result.Error();
                    return IterationResult::eBreak;
                }

                return IterationResult::eContinue;
            });

        if (status) return Error(status);
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
    ErrorOr<void> Loader::ParseSegment(const ProgramHeader& segment)
    {
        // TODO(v1tr10l7): correct permissions
        // TODO(v1tr10l7): EntryPoint
        // TODO(v1tr10l7): TLS
        // TODO(v1tr10l7): INTERP
        // TODO(v1tr10l7): DYNAMIC
        // TODO(v1tr10l7): AUX

        switch (segment.Type)
        {
            case HeaderType::eDynamic: return ParseDynamic(segment);
            case HeaderType::eLoad: return LoadSegment(segment);

            default: break;
        }

        return {};
    }
    ErrorOr<void> Loader::ParseDynamic(const ProgramHeader& segment)
    {
        usize      sizeInFile   = segment.SegmentSizeInFile;

        const auto dynamicTable = reinterpret_cast<DynamicEntry*>(
            m_Image->Raw().Offset(segment.Offset));
        for (usize i = 0; i < sizeInFile / sizeof(DynamicEntry); i++)
        {
            const auto& entry = dynamicTable[i];
            switch (entry.Tag)
            {
                case DynamicEntryType::eNull: break;
                case DynamicEntryType::eNeeded:
                    LogDebug("ELF::Loader: DT_NEEDED => {}",
                             m_Image->LookupString(entry.Data.Value));
                    break;
                case DynamicEntryType::eStringTable: break;
                case DynamicEntryType::eStringTableSize: break;
                case DynamicEntryType::eInitArray:
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

        return {};
    }
    ErrorOr<void> Loader::LoadSegment(const ProgramHeader& segment)
    {
        usize   sizeInFile       = segment.SegmentSizeInFile;
        usize   sizeInMemory     = segment.SegmentSizeInMemory;

        Pointer segmentStartVirt = m_Bias.Offset(segment.VirtualAddress);
        Pointer segmentEndVirt   = segmentStartVirt.Offset(sizeInFile);
        Pointer imageStartVirt   = m_Image->IsShared() ? m_LoadBase : m_MinVirt;
        Assert(segmentEndVirt <= imageStartVirt.Offset(m_Size));

        usize offsetInFile  = segment.Offset;
        usize offsetInImage = segment.VirtualAddress - m_MinVirt.Raw();
        usize offsetInPage  = segmentStartVirt.Raw()
                           - Math::AlignDown(segmentStartVirt, PMM::PAGE_SIZE);
        Pointer segmentPhys = m_Phys.Offset(offsetInImage + m_VirtMisalign);

        // sanity: segment fits inside allocated physical block
        usize   totalSegmentSize
            = Math::AlignUp(offsetInPage + sizeInMemory, PMM::PAGE_SIZE);
        if (segmentPhys.Offset(sizeInMemory)
            > m_Phys.Offset(m_AlignedSize + m_VirtMisalign))
        {
            LogError(
                "ELF::Loader: segment would overflow phys block (rel={:#x}, "
                "needed={:#x})",
                offsetInImage, totalSegmentSize);
            return {};
        }

#if CTOS_ELF_DEBUG
        LogDebug("ELF::Loader: sizeInFile => {:#x}, sizeInMemory => {:#x}",
                 sizeInFile, sizeInMemory);
        LogDebug(
            "ELF::Loader: Loading segment at {:#x} offset in file, "
            "{:#x} offset in page, and {:#x} offset in image",
            offsetInFile, offsetInPage, offsetInImage);
        LogDebug("ELF::Loader: segment => { .Physical: {:#x}, .Virtual: {:#x}",
                 segmentPhys, segmentStartVirt);

        LogWarn("ELF::Loader: Copy =>");
        LogWarn("ELF::Loader: physDest.ToHigherHalf() => {:#x}",
                segmentPhys.ToHigherHalf());
        LogWarn("ELF::Loader: Offset => {:#x}", offsetInFile);
        LogWarn("ELF::Loader: sizeInFile => {:#x}", sizeInFile);
#endif
        auto pte = m_PageMap->Virt2Pte(m_PageMap->TopLevel(), segmentStartVirt,
                                       false, PMM::PAGE_SIZE);
        auto attributes = Arch::VMM::FromNativeFlags(pte->Flags());
        if (segment.Attributes & SegmentAttributes::eReadable)
            attributes |= PageAttributes::eRead;
        else if (segment.Attributes & SegmentAttributes::eWriteable)
            attributes |= PageAttributes::eWrite;
        else if (segment.Attributes & SegmentAttributes::eExecutable)
            attributes |= PageAttributes::eExecutable;

        if (segmentStartVirt.Raw() == 0x6ffffbff000
            || AddressRange(segmentStartVirt.Raw(), sizeInMemory)
                   .Contains(0x6ffffbff000ull))
            LogError("ELF::Loader: found");

        if (!m_PageMap->ProtectRange(segmentStartVirt, sizeInMemory,
                                     attributes))
            LogError(
                "ELF::Loader: Failed to set protection attributes for "
                "virtual "
                "region at {:#x}-{:#x}",
                segmentStartVirt, segmentEndVirt);
        Memory::Copy(segmentPhys.ToHigherHalf(),
                     m_Image->Raw().Offset(offsetInFile), sizeInFile);

        // Zero out BSS (bytes between filesz and memsz)
        if (sizeInMemory > sizeInFile)
        {
            Pointer bssStart = segmentPhys.Offset(sizeInFile);
            usize   bssLen   = sizeInMemory - sizeInFile;

#if CTOS_ELF_DEBUG
            LogWarn("ELF::Loader: Fill =>");
            LogWarn("ELF::Loader: zeroStart.ToHigherHalf() => {:#x}",
                    bssStart.ToHigherHalf());
            LogWarn("ELF::Loader: zeroLen => {:#x}", bssLen);
#endif
            Memory::Fill(bssStart.ToHigherHalf(), 0, bssLen);
        }

        return {};
    }
}; // namespace ELF
