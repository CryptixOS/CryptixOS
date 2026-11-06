/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Syscall.hpp>

#include <Memory/Allocator/KernelHeap.hpp>
#include <Memory/MM.hpp>
#include <Scheduler/Process.hpp>

#include <Prism/String/Formatter.hpp>
#include <VFS/FileDescriptor.hpp>

#include <icxxabi>
#include <limine.h>

namespace PMM
{
    void EarlyInitialize(const MemoryMap&);
};
namespace MM
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

        // The heap runs on top of the PMM's early (memory map based) page
        // allocator until PMM::Initialize() sets up the bitmap.
        PMM::EarlyInitialize(s_MemoryMap);
        KernelHeap::Initialize();

        auto pagingModeString = ToString(s_PagingMode);
        pagingModeString.RemovePrefix(1);

        LogTrace(
            "MM: Memory Information =>\n"
            "\tKernel Physical Address => {:#p}\n"
            "\tKernel Virtual Address => {:#p}\n"
            "\tHigher Half Direct Mapping Offset => {:#x}\n"
            "\tPaging Mode => {}\n",
            s_KernelPhysicalAddress,
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
        if (!region)
        {
            VMM::FreeSpace(virt);
            PMM::FreePages(phys, pageCount);
            return nullptr;
        }
        region->SetAccessMode(access);

        if (!VMM::MapKernelRegion(region->VirtualBase(), region->PhysicalBase(),
                                  pageCount, attributes))
        {
            VMM::FreeSpace(virt);
            PMM::FreePages(phys, pageCount);
            return nullptr;
        }

        s_KernelAddressSpace.Insert(virt, region);
        return region;
    }

    Ref<Region> AllocateKernelRegion(const usize bytes, PageAttributes flags)
    {
        auto region = VMM::AllocateKernelRegion(bytes, flags);
        if (!region) return nullptr;

        usize pageCount = Math::DivRoundUp(bytes, PMM::PAGE_SIZE);
        auto  phys      = PMM::CallocatePages(pageCount);
        if (!phys)
        {
            VMM::FreeSpace(region->VirtualBase());
            return nullptr;
        }

        // A kernel region must not be accessible from user mode.
        using VMM::Access;
        Access access = Access::eNone;
        if (flags & PageAttributes::eRead) access |= Access::eRead;
        if (flags & PageAttributes::eWrite) access |= Access::eWrite;
        if (flags & PageAttributes::eExecutable) access |= Access::eExecute;

        region->SetPhysicalBase(phys);
        region->SetAccessMode(access);

        auto pageMap = VMM::GetKernelPageMap();
        if (!pageMap->MapRegion(region))
        {
            PMM::FreePages(phys, pageCount);
            VMM::FreeSpace(region->VirtualBase());
            return nullptr;
        }

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
        if (!phys)
        {
            addressSpace.Erase(region->VirtualBase());
            return nullptr;
        }

        using VMM::Access;
        Access access = Access::eUser;
        if (flags & PageAttributes::eRead) access |= Access::eRead;
        if (flags & PageAttributes::eWrite) access |= Access::eWrite;
        if (flags & PageAttributes::eExecutable) access |= Access::eExecute;

        region->SetPhysicalBase(phys);
        region->SetAccessMode(access);

        auto pageMap = process->PageMap;
        if (!pageMap->MapRegion(region))
        {
            PMM::FreePages(phys, pageCount);
            addressSpace.Erase(region->VirtualBase());
            return nullptr;
        }

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
        if (reason & PageFaultReason::eNotPresent)
            message += "\t- Non-present page\n";
        if (reason & PageFaultReason::eWrite)
            message += "\t- Write violation\n";
        else message += "\t- Read violation\n";
        if (reason & PageFaultReason::eReservedWrite)
            message += "\t- Reserved bit set in a paging structure\n";
        if (reason & PageFaultReason::eInstructionFetch)
            message += "\t- Instruction Fetch\n";
        if (reason & PageFaultReason::eProtectionKey)
            message += "\t- Protection-Key violation\n";
        if (reason & PageFaultReason::eShadowStack)
            message += "\t- Shadow stack access\n";
        if (reason & PageFaultReason::eSoftwareGuardExtension)
            message += "\t- SGX violation\n";

        bool kernelFault = !(reason & PageFaultReason::eUser);
        message += Format("In {} space", kernelFault ? "Kernel" : "User");

        auto        process = Process::Current();
        Ref<Region> region  = nullptr;

        // Idle processes have no address space.
        if (process && process->HasAddressSpace())
        {
            auto& addressSpace = process->AddressSpace();
            region             = addressSpace.Find(info.VirtualAddress());
        }

        // Demand-allocate a lazily created region on its first access. Only a
        // *not-present* fault on a region with no backing memory yet qualifies;
        // a protection fault (or a fault on an already backed region) is a real
        // access violation and must fall through to the error path below.
        if (region && (reason & PageFaultReason::eNotPresent)
            && !region->PhysicalBase())
        {
            Assert(region->VirtualBase());

            // The backing allocation must cover the whole region, because
            // that's what MapRegion() maps -- sizing it from the file would
            // expose the physical pages that follow it.
            usize   regionSize = region->Size();
            usize   pageCount  = Math::DivRoundUp(regionSize, PMM::PAGE_SIZE);
            Pointer phys       = PMM::CallocatePages(pageCount);

            if (phys)
            {
                region->SetPhysicalBase(phys);

                auto fd   = region->FileDescriptor();
                auto file = fd ? fd->File() : nullptr;
                if (file)
                {
                    LogTrace("MM: Mapping file...");

                    // Read the region's slice of the file at its own offset;
                    // the descriptor's position must not be used or modified.
                    usize fileSize = file->Size();
                    usize offset   = region->FileOffset();
                    usize toRead
                        = offset < fileSize ? Min(fileSize - offset, regionSize)
                                            : 0;

                    if (toRead)
                    {
                        auto result
                            = file->Read(phys.ToHigherHalf<void*>(),
                                         static_cast<off_t>(offset), toRead);
                        if (!result)
                            LogError("MM: Failed to read the file descriptor");
                        else if (*result != static_cast<isize>(toRead))
                            LogError(
                                "MM: Short read while mapping a file: {} of {} "
                                "bytes",
                                *result, toRead);
                    }
                }

                auto attributes = region->PageAttributes();
                // Regions of a process' address space are user mappings.
                if (region->VirtualBase() < Pointer(HigherHalfOffset()))
                    attributes |= PageAttributes::eUser;

                auto pageMap = process->PageMap;
                if (pageMap->MapRange(region->VirtualBase(), phys,
                                      pageCount * PMM::PAGE_SIZE, attributes))
                    return;

                region->SetPhysicalBase(nullptr);
                PMM::FreePages(phys.As<void>(), pageCount);
            }
            errno = ENOMEM;
        }

        if (CPU::DuringSyscall())
        {
            usize      syscallID   = CPU::GetCurrent()->LastSyscallID;
            StringView syscallName = Syscall::GetName(syscallID);
            message += Format(", and during syscall({}) => {}", syscallID,
                              syscallName);
        }

        // TODO(v1tr10l7): Dump thread's stacktrace, if symbols are available
        message += '\n';
        if (process && !kernelFault)
        {
            auto tty = process->TTY();
            LogError("{}: Segmentation Fault(core dumped)\n{}", process->Name(),
                     message);
            Stacktrace::Print();

            // `message` lives in kernel memory, so it is written through the
            // kernel-pointer overload. (Wrapping it in a UserBuffer failed the
            // user-range check and returned *before* Exit(), so the process
            // resumed at the faulting instruction and faulted forever.)
            if (tty)
            {
                auto status = tty->Write(message.data(), 0, message.size());
                (void)status;
            }
            process->Exit(-1);
            return;
        }

        earlyPanic(message.data());
    }
}; // namespace MM
