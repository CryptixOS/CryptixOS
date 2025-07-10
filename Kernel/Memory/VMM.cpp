/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>
#include <Arch/CPU.hpp>

#include <Boot/BootInfo.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Scheduler/Process.hpp>

extern "C" symbol      text_start_addr;
extern "C" symbol      text_end_addr;
extern "C" symbol      kernel_init_start_addr;
extern "C" symbol      kernel_init_end_addr;
extern "C" symbol      module_init_start_addr;
extern "C" symbol      module_init_end_addr;
extern "C" symbol      rodata_start_addr;
extern "C" symbol      rodata_end_addr;
extern "C" symbol      data_start_addr;
extern "C" symbol      data_end_addr;

static PageMap*        s_KernelPageMap;
static Pointer         s_VirtualAddressSpace{};

static Optional<usize> s_HigherHalfOffset = BootInfo::GetHHDMOffset();

using namespace Arch::VMM;

void* PageMap::NextLevel(PageTableEntry& entry, bool allocate, uintptr_t virt)
{
    if (entry.IsValid()) return Pointer(entry.Address()).ToHigherHalf<void*>();
    if (!allocate) return nullptr;

    Pointer newEntry = Arch::VMM::AllocatePageTable();
    entry.SetAddress(newEntry.FromHigherHalf());
    entry.SetFlags(Arch::VMM::g_DefaultPteFlags, true);
    return newEntry;
}

namespace VMM
{
    static bool s_Initialized = false;

    void        Initialize()
    {
        if (s_Initialized) return;
        s_Initialized = true;
        LogTrace("VMM: Initializing...");
        Arch::VMM::Initialize();

        s_KernelPageMap = new PageMap();
        Assert(s_KernelPageMap->TopLevel() != 0);

        auto [pageSize, flags] = s_KernelPageMap->RequiredSize(4_gib);

        Assert(s_KernelPageMap->MapRange(GetHigherHalfOffset(), 0, 4_gib,
                                         PageAttributes::eRW | flags
                                             | PageAttributes::eWriteBack));

        auto memoryMap = BootInfo::MemoryMap();
        for (usize i = 0; i < memoryMap.EntryCount; i++)
        {
            auto&   entry = memoryMap.Entries[i];

            Pointer base  = Math::AlignDown(entry.Base(), GetPageSize());
            Pointer top   = Math::AlignUp(entry.Base().Offset(entry.Size()),
                                          GetPageSize());

            auto    size  = (top - base).Raw();
            auto [pageSize, flags] = s_KernelPageMap->RequiredSize(size);

            auto alignedSize       = Math::AlignDown(size, pageSize);

            flags |= PageAttributes::eWriteBack;
            if (entry.Type() == MemoryType::eFramebuffer)
                flags |= PageAttributes::eWriteCombining;

            if (base < 4_gib) continue;
            Assert(s_KernelPageMap->MapRange(
                base.ToHigherHalf(), base, alignedSize,
                PageAttributes::eRW | flags | PageAttributes::eWriteBack));
            base += alignedSize;

            Assert(s_KernelPageMap->MapRange(
                base.ToHigherHalf(), base, size - alignedSize,
                PageAttributes::eRW | PageAttributes::eWriteBack));
        }

        auto kernelExecutable = BootInfo::ExecutableFile();
        auto kernelPhys       = BootInfo::KernelPhysicalAddress();
        auto kernelVirt       = BootInfo::KernelVirtualAddress();

        Assert(s_KernelPageMap->MapRange(
            kernelVirt, kernelPhys, kernelExecutable->size,
            PageAttributes::eRWX | PageAttributes::eWriteBack));

        auto textVirt = Pointer(text_start_addr);
        auto textPhys = Pointer(text_start_addr) - kernelVirt + kernelPhys;
        auto textSize = Pointer(text_end_addr) - textVirt;
        Assert(s_KernelPageMap->MapRange(textVirt, textPhys, textSize,
                                         PageAttributes::eRWX
                                             | PageAttributes::eWriteBack));

        {
            auto memoryTop = PMM::GetMemoryTop();
            auto base = Pointer(Math::AlignUp(memoryTop, 1_gib)).ToHigherHalf();
            s_VirtualAddressSpace = base.Offset(4_gib);
        }

        auto modulesVirt = Pointer(module_init_start_addr);
        auto modulesPhys = modulesVirt - kernelVirt + kernelPhys;
        auto modulesSize = Pointer(module_init_end_addr) - modulesVirt;
        s_KernelPageMap->MapRange(modulesVirt, modulesPhys, modulesSize,
                                  PageAttributes::eRWX
                                      | PageAttributes::eWriteBack);

        s_KernelPageMap->Load();
        LogInfo("VMM: Loaded kernel page map");
    }
    void UnmapKernelInitCode()
    {
        auto initVirt = Pointer(kernel_init_start_addr);
        auto initSize = Pointer(kernel_init_end_addr) - initVirt;
        s_KernelPageMap->UnmapRange(initVirt, initSize);
    }

    usize   GetHigherHalfOffset() { return s_HigherHalfOffset.ValueOr(0); }

    Pointer AllocateSpace(usize increment, usize alignment, bool lowerHalf)
    {
        Assert(alignment <= PMM::PAGE_SIZE);

        if (alignment == 0) alignment = sizeof(void*);
        Pointer virt = Math::AlignUp(s_VirtualAddressSpace, alignment);

        s_VirtualAddressSpace
            += (virt - s_VirtualAddressSpace).Offset(increment);

        return lowerHalf ? virt.FromHigherHalf() : virt.ToHigherHalf<>();
    }

    Ref<Region> AllocateDMACoherent(usize size, PageAttributes flags)
    {
        usize pageCount = Math::DivRoundUp(size, PMM::PAGE_SIZE);
        auto  pages     = PMM::CallocatePages(pageCount);

        auto  virt = AllocateSpace(pageCount * PMM::PAGE_SIZE, 4_kib, true);
        Assert(MapKernelRegion(virt, pages, pageCount, flags));

        auto region
            = CreateRef<Region>(pages, virt, pageCount * PMM::PAGE_SIZE);
        if (!region) return nullptr;

        region->SetAccessMode(Access::eReadWrite);
        return region;
    }
    void FreeDMA_Region(Ref<Region> region)
    {
        // TODO(v1tr10l7): we should keep track of mapped kernel addresses
        s_KernelPageMap->UnmapRange(region->VirtualBase(), region->Size());
    }

    PageMap* GetKernelPageMap()
    {
        if (!s_Initialized) Initialize();
        return s_KernelPageMap;
    }

    bool MapKernelRegion(Pointer virt, Pointer phys, usize pageCount,
                         PageAttributes attributes)
    {
        return MapKernelRange(virt, phys, pageCount * PMM::PAGE_SIZE,
                              attributes);
    }
    bool MapKernelRange(Pointer virt, Pointer phys, usize size,
                        PageAttributes attributes)
    {
        if (!s_KernelPageMap) return false;

        return s_KernelPageMap->MapRange(virt, phys, size, attributes);
    }
    bool UnmapKernelRange(Pointer virt, usize size, PageAttributes flags)
    {
        return s_KernelPageMap->UnmapRange(virt, size, flags);
    }

    Pointer MapIoRegion(PhysAddr phys, usize size, bool write, usize alignment)
    {
        auto           virt = AllocateSpace(size, alignment, true);
        PageAttributes attributes
            = PageAttributes::eRead | PageAttributes::eUncacheableStrong;
        if (write) attributes |= PageAttributes::eWrite;

        if (!s_KernelPageMap->MapRange(virt, phys, size, attributes))
            return nullptr;

        return virt;
    }
} // namespace VMM
