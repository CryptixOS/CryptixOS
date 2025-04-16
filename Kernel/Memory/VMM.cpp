/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <Scheduler/Process.hpp>

extern "C" symbol           text_start_addr;
extern "C" symbol           text_end_addr;
extern "C" symbol           rodata_start_addr;
extern "C" symbol           rodata_end_addr;
extern "C" symbol           data_start_addr;
extern "C" symbol           data_end_addr;

static PageMap*             s_KernelPageMap;
static Pointer              s_VirtualAddressSpace{};

static std::optional<usize> s_HigherHalfOffset = BootInfo::GetHHDMOffset();

using namespace Arch::VMM;

void* PageMap::GetNextLevel(PageTableEntry& entry, bool allocate,
                            uintptr_t virt)
{
    if (entry.IsValid())
        return Pointer(entry.GetAddress()).ToHigherHalf<void*>();
    if (!allocate) return nullptr;

    Pointer newEntry = Arch::VMM::AllocatePageTable();
    entry.SetAddress(newEntry.FromHigherHalf());
    entry.SetFlags(Arch::VMM::g_DefaultPteFlags, true);
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

        s_KernelPageMap = new PageMap();
        Assert(s_KernelPageMap->GetTopLevel() != 0);

        auto [pageSize, flags] = s_KernelPageMap->RequiredSize(4_gib);

        Assert(s_KernelPageMap->MapRange(GetHigherHalfOffset(), 0, 4_gib,
                                         PageAttributes::eRW | flags
                                             | PageAttributes::eWriteBack));

        usize entryCount    = 0;
        auto  memoryEntries = BootInfo::GetMemoryMap(entryCount);
        for (usize i = 0; i < entryCount; i++)
        {
            MemoryMapEntry* entry = memoryEntries[i];

            Pointer         base  = Math::AlignDown(entry->base, GetPageSize());
            Pointer         top
                = Math::AlignUp(entry->base + entry->length, GetPageSize());

            auto size              = (top - base).Raw();
            auto [pageSize, flags] = s_KernelPageMap->RequiredSize(size);

            auto alignedSize       = Math::AlignDown(size, pageSize);

            flags |= PageAttributes::eWriteBack;
            if (entry->type == MEMORY_MAP_FRAMEBUFFER)
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

        auto kernelExecutable = BootInfo::GetExecutableFile();
        auto kernelPhys       = BootInfo::GetKernelPhysicalAddress();
        auto kernelVirt       = BootInfo::GetKernelVirtualAddress();

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

        s_KernelPageMap->Load();
        LogInfo("VMM: Loaded kernel page map");
    }
    usize GetHigherHalfOffset()
    {
        return s_HigherHalfOffset.has_value() ? s_HigherHalfOffset.value() : 0;
    }

    Pointer AllocateSpace(usize increment, usize alignment, bool lowerHalf)
    {
        Assert(alignment <= PMM::PAGE_SIZE);

        if (alignment == 0) alignment = sizeof(void*);
        Pointer virt = Math::AlignUp(s_VirtualAddressSpace, alignment);

        s_VirtualAddressSpace
            += (virt - s_VirtualAddressSpace).Offset(increment);

        return lowerHalf ? virt.FromHigherHalf() : virt.ToHigherHalf<>();
    }

    PageMap* GetKernelPageMap()
    {
        if (!s_Initialized) Initialize();
        return s_KernelPageMap;
    }
    void HandlePageFault(const PageFaultInfo& info)
    {
        auto message
            = std::format("Page Fault occurred at '{:#x}'\nCaused by:\n",
                          info.VirtualAddress().Raw());
        auto reason = info.Reason();

        if (reason & PageFaultReason::eNotPresent)
            message += "\t- Non-present page\n";
        if (reason & PageFaultReason::eWrite)
            message += "\t- Write violation\n";
        else message += "\t- Read violation\n";
        if (reason & PageFaultReason::eUser) message += "\t- Reserved Write\n";
        if (reason & PageFaultReason::eInstructionFetch)
            message += "\t- Instruction Fetch\n";
        if (reason & PageFaultReason::eProtectionKey)
            message += "\t- Protection-Key violation\n";
        if (reason & PageFaultReason::eShadowStack)
            message += "\t- Shadow stack access\n";
        if (reason & PageFaultReason::eSoftwareGuardExtension)
            message += "\t- SGX violation\n";

        bool kernelFault = !(reason & PageFaultReason::eUser);
        message
            += std::format("In {} space\n", kernelFault ? "User" : "Kernel");

        auto process = Process::GetCurrent();

        if (process && !kernelFault)
        {
            auto tty = process->GetTTY();
            LogError("{}: Segmentation Fault(core dumped)", process->GetName());
            Stacktrace::Print();

            if (tty) tty->Write(message.data(), 0, message.size());
            process->Exit(-1);
            return;
        }

        panic(message);
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
} // namespace VirtualMemoryManager
