/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>

#include <Memory/Allocator/KernelHeap.hpp>
#include <Memory/MemoryManager.hpp>
#include <Scheduler/Process.hpp>

#include <Prism/String/Formatter.hpp>

#include <icxxabi>
#include <limine.h>

namespace KernelHeap
{
    void EarlyInitialize(Pointer base, usize length);
};
namespace PhysicalMemoryManager
{
    void EarlyInitialize(const MemoryMap&);
};
namespace MemoryManager
{
    namespace
    {
        AddressSpace    s_KernelAddressSpace;
        Pointer         s_KernelPhysicalAddress = nullptr;
        Pointer         s_KernelVirtualAddress  = nullptr;
        usize           s_HigherHalfOffset      = 0;
        enum PagingMode s_PagingMode            = PagingMode::eNone;
        MemoryMap       s_MemoryMap;
        EfiMemoryMap    s_EfiMemoryMap;
    }; // namespace

    void PrepareInitialHeap(const BootMemoryInfo& memoryInfo)
    {
        s_KernelPhysicalAddress = memoryInfo.KernelPhysicalBase;
        s_KernelVirtualAddress  = memoryInfo.KernelVirtualBase;
        s_HigherHalfOffset      = memoryInfo.HigherHalfOffset;
        s_PagingMode            = memoryInfo.PagingMode;
        s_MemoryMap             = memoryInfo.MemoryMap;
        s_EfiMemoryMap          = memoryInfo.EfiMemoryMap;

        auto allocate           = [&](usize bytes) -> Pointer
        {
            for (usize i = 0; i < s_MemoryMap.EntryCount; i++)
            {
                auto& entry = s_MemoryMap.Entries[i];
                if (entry.Type() == MemoryZoneType::eUsable
                    && entry.Length() > bytes)
                    return entry.Allocate(bytes);
            }

            return nullptr;
        };

        // Initialize early kernel heap
        auto earlyHeapSize = 64_mib;
        auto earlyHeapBase = allocate(earlyHeapSize);

        KernelHeap::EarlyInitialize(earlyHeapBase, earlyHeapSize);
        PMM::EarlyInitialize(s_MemoryMap);
        KernelHeap::Initialize();

        auto pagingModeString = ToString(s_PagingMode);
        pagingModeString.RemovePrefix(1);

        LogTrace(
            "MemoryManager: Memory Information =>\n"
            "\tKernel Physical Address => {:#p}\n"
            "\tKernel Virtual Address => {:#p}\n"
            "\tHigher Half Direct Mapping Offset => {:#p}\n",
            "\tPaging Mode => {}\n", s_KernelPhysicalAddress,
            s_KernelVirtualAddress, s_HigherHalfOffset, pagingModeString);
    }
    void Initialize()
    {
        // Call global constructors
        icxxabi::Initialize();
        // Initialize Physical Memory Manager
        Assert(PMM::Initialize(s_MemoryMap));

        VMM::Initialize(s_KernelPhysicalAddress, s_KernelVirtualAddress,
                        s_HigherHalfOffset);
    }

    Pointer         KernelPhysicalAddress() { return s_KernelPhysicalAddress; }
    Pointer         KernelVirtualAddress() { return s_KernelVirtualAddress; }
    usize           HigherHalfOffset() { return s_HigherHalfOffset; }
    enum PagingMode PagingMode() { return s_PagingMode; }

    Ref<Region>     AllocateRegion(const usize bytes, PageAttributes attributes,
                                   const MemoryUsage memoryUsage)
    {
        switch (memoryUsage)
        {
            case MemoryUsage::eGeneric:
            case MemoryUsage::eKernelHeap:
            case MemoryUsage::eModule:
            case MemoryUsage::eDMA:
            case MemoryUsage::ePageTable:
            case MemoryUsage::eDevice: break;

            case MemoryUsage::eUser:
                return AllocateUserRegion(bytes, attributes);

            default: break;
        }

        usize pageCount = Math::DivRoundUp(bytes, PMM::PAGE_SIZE);
        auto  phys      = PMM::CallocatePages(pageCount);
        if (!phys) return nullptr;

        auto virt = VMM::AllocateSpace(bytes, 0, false);
        if (!virt)
        {
            PMM::FreePages(phys, pageCount);
            return nullptr;
        }

        using VMM::Access;
        Access access = Access::eNone;
        if (attributes & PageAttributes::eRead) access |= Access::eRead;
        if (attributes & PageAttributes::eWrite) access |= Access::eWrite;
        if (attributes & PageAttributes::eExecutable)
            access |= Access::eExecute;

        auto region = CreateRef<Region>(phys, virt, pageCount * PMM::PAGE_SIZE);
        if (!region) return nullptr;
        region->SetAccessMode(access);

        if (!VMM::MapKernelRegion(region->VirtualBase(), region->PhysicalBase(),
                                  pageCount, attributes))
        {
            PMM::FreePages(phys, pageCount);
            return nullptr;
        }

        s_KernelAddressSpace.Insert(virt, region);
        return region;
    }
    Ref<Region> AllocateUserRegion(const usize bytes, PageAttributes flags)
    {
        auto  process      = Process::Current();
        auto& addressSpace = process->AddressSpace();

        auto  region       = addressSpace.AllocateRegion(bytes, 0);
        if (!region) return nullptr;

        usize pageCount = Math::DivRoundUp(bytes, PMM::PAGE_SIZE);
        auto  phys      = PMM::CallocatePages(pageCount);
        if (!phys) return nullptr;

        using VMM::Access;
        Access access = Access::eUser;
        if (flags & PageAttributes::eRead) access |= Access::eRead;
        if (flags & PageAttributes::eWrite) access |= Access::eWrite;
        if (flags & PageAttributes::eExecutable) access |= Access::eExecute;

        region->SetPhysicalBase(phys);
        region->SetAccessMode(access);

        auto pageMap = process->PageMap;
        pageMap->MapRegion(region);

        return region;
    }

    void FreeRegion(Ref<Region> region)
    {
        // TODO(v1tr10l7): Free region
    }

    void HandlePageFault(const PageFaultInfo& info)
    {
        auto message = Format("Page Fault occurred at '{:#x}'\nCaused by:\n",
                              info.VirtualAddress().Raw());
        auto reason  = info.Reason();

        auto process = Process::Current();
        Ref<Region> region = nullptr;

        if (process)
        {
            auto& addressSpace = process->AddressSpace();
            region             = addressSpace.Find(info.VirtualAddress());
        }

        if (region)
        {
            usize pageCount = Math::DivRoundUp(region->Size(), PMM::PAGE_SIZE);
            auto  phys      = PMM::CallocatePages(pageCount);

            if (phys)
            {
                region->SetPhysicalBase(phys);

                auto pageMap = process->PageMap;
                pageMap->MapRegion(region);
                return;
            }

            errno = ENOMEM;
        }

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
        message += Format("In {} space", kernelFault ? "User" : "Kernel");

        if (CPU::DuringSyscall())
        {
            usize      syscallID   = CPU::GetCurrent()->LastSyscallID;
            StringView syscallName = Syscall::GetName(syscallID);
            message += Format(", and during syscall({}) => {}", syscallID,
                              syscallName);
        }

        //TODO(v1tr10l7): Dump thread's stacktrace, if symbols are available
        message += '\n';
        if (process && !kernelFault)
        {
            auto tty = process->TTY();
            LogError("{}: Segmentation Fault(core dumped)\n{}", process->Name(),
                     message);
            Stacktrace::Print();

            auto bufferOr
                = UserBuffer::ForUserBuffer(message.data(), message.size());
            if (!bufferOr) return;
            auto messageBuffer = bufferOr.value();
            if (tty)
            {
                auto status = tty->Write(messageBuffer, 0, message.size());
                (void)status;
            }
            process->Exit(-1);
            return;
        }

        earlyPanic(message.data());
    }
}; // namespace MemoryManager
