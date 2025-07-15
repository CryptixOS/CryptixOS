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

namespace MemoryManager
{
    namespace
    {
        AddressSpace s_KernelAddressSpace;
        Pointer      s_KernelPhysicalAddress = nullptr;
        Pointer      s_KernelVirtualAddress  = nullptr;
    }; // namespace

    void Initialize(const MemoryMap& memoryMap, Pointer kernelPhys,
                    Pointer kernelVirt, usize higherHalfOffset)
    {
        s_KernelPhysicalAddress = kernelPhys;
        s_KernelVirtualAddress  = kernelVirt;

        // Initialize early kernel heap
        KernelHeap::Initialize();
        // Call global constructors
        icxxabi::Initialize();
        // Initialize Physical Memory Manager
        Assert(PMM::Initialize(memoryMap));

        VMM::Initialize(kernelPhys, kernelVirt, higherHalfOffset);
    }

    Pointer     KernelPhysicalAddress() { return s_KernelPhysicalAddress; }
    Pointer     KernelVirtualAddress() { return s_KernelVirtualAddress; }

    Ref<Region> AllocateRegion(const usize bytes, PageAttributes attributes,
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
