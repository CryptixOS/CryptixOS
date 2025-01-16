/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "VMM.hpp"

#include "Memory/PMM.hpp"

#include "Boot/BootInfo.hpp"
#include "Utility/Math.hpp"

extern "C" symbol text_start_addr;
extern "C" symbol text_end_addr;
extern "C" symbol rodata_start_addr;
extern "C" symbol rodata_end_addr;
extern "C" symbol data_start_addr;
extern "C" symbol data_end_addr;

static PageMap*   kernelPageMap;
static uintptr_t  virtualAddressSpace{};

using namespace Arch::VMM;

void* PageMap::GetNextLevel(PageTableEntry& entry, bool allocate,
                            uintptr_t virt)
{
    if (entry.IsValid())
        return ToHigherHalfAddress<void*>(
            static_cast<uintptr_t>(entry.GetAddress()));

    if (!allocate) return nullptr;

    auto newEntry = Arch::VMM::AllocatePageTable();
    entry.SetAddress(FromHigherHalfAddress<uintptr_t>(
        reinterpret_cast<uintptr_t>(newEntry)));
    entry.SetFlags(Arch::VMM::defaultPteFlags, true);
    return newEntry;
}

namespace VirtualMemoryManager
{
    static bool s_Initialized = false;

    void        Initialize()
    {
        if (s_Initialized) return;
        s_Initialized = true;
        LogTrace("VMM: Initializing...");
        Arch::VMM::Initialize();

        kernelPageMap = new PageMap();
        Assert(kernelPageMap->GetTopLevel() != 0);

        auto [pageSize, flags] = kernelPageMap->RequiredSize(4_gib);
        for (uintptr_t i = 0; i < 1_gib * 4; i += pageSize)
            Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(i), i,
                                      PageAttributes::eRW | flags
                                          | PageAttributes::eWriteBack));

        usize entryCount = 0;
        auto  entries    = BootInfo::GetMemoryMap(entryCount);
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* entry = entries[i];

            uintptr_t       base  = Math::AlignDown(entry->base, GetPageSize());
            uintptr_t       top
                = Math::AlignUp(entry->base + entry->length, GetPageSize());

            auto size              = top - base;
            auto [pageSize, flags] = kernelPageMap->RequiredSize(size);

            auto alignedSize       = Math::AlignDown(size, pageSize);

            flags |= PageAttributes::eWriteBack;
            if (entry->type == MEMORY_MAP_FRAMEBUFFER)
                flags |= PageAttributes::eWriteCombining;

            for (uintptr_t t = base; t < (base + alignedSize); t += pageSize)
            {
                if (t < 4_gib) continue;

                Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(t), t,
                                          PageAttributes::eRW | flags
                                              | PageAttributes::eWriteBack));
            }
            base += alignedSize;

            for (uintptr_t t = base; t < (base + size - alignedSize);
                 t += GetPageSize())
            {
                if (t < 4_gib) continue;

                Assert(kernelPageMap->Map(ToHigherHalfAddress<uintptr_t>(t), t,
                                          PageAttributes::eRW
                                              | PageAttributes::eWriteBack));
            }
        }

        for (usize i = 0; i < BootInfo::GetKernelFile()->size;
             i += GetPageSize())
        {
            uintptr_t phys = BootInfo::GetKernelPhysicalAddress().Raw<>() + i;
            uintptr_t virt = BootInfo::GetKernelVirtualAddress().Raw<>() + i;
            Assert(kernelPageMap->Map(
                virt, phys, PageAttributes::eRWX | PageAttributes::eWriteBack));
        }

        {
            auto base = ToHigherHalfAddress<uintptr_t>(
                Math::AlignUp(PMM::GetMemoryTop(), 1_gib));
            virtualAddressSpace = base + 4_gib;
        }

        kernelPageMap->Load();
        LogInfo("VMM: Loaded kernel page map");
    }

    uintptr_t AllocateSpace(usize increment, usize alignment, bool lowerHalf)
    {
        Assert(alignment <= PMM::PAGE_SIZE);

        uintptr_t ret = alignment
                          ? Math::AlignUp(virtualAddressSpace, alignment)
                          : virtualAddressSpace;
        virtualAddressSpace += increment + (ret - virtualAddressSpace);

        return lowerHalf ? FromHigherHalfAddress<uintptr_t>(ret) : ret;
    }

    PageMap* GetKernelPageMap()
    {
        if (!s_Initialized) Initialize();
        return kernelPageMap;
    }
} // namespace VirtualMemoryManager
