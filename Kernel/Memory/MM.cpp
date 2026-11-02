/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Syscall.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>
#include <Prism/String/Formatter.hpp>
#include <limine.h>

module Cryptix:Impl.Memory.MM;

import :API.Syscall;
import :Memory.Allocator.KernelHeap;
import :Memory.MM;
import :Scheduler.Process;
import :VFS.FileDescriptor;
import :Library.ICxxAbi;

namespace KernelHeap
{
    void EarlyInitialize(Pointer base, usize length);
    void MapEarlyHeap();
}; // namespace KernelHeap
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

        auto allocate           = [&](usize bytes) -> Pointer
        {
            for (usize i = 0; i < s_MemoryMap.EntryCount; i++)
            {
                auto& entry    = s_MemoryMap.Entries[i];
                auto  base     = Math::AlignUp(entry.Base(), PMM::PAGE_SIZE);
                usize misalign = base - entry.Base().Raw();

                if (entry.Type() == MemoryZoneType::eUsable
                    && entry.Length() > bytes + misalign)
                    return Math::AlignUp(entry.Allocate(bytes + misalign),
                                         PMM::PAGE_SIZE);
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
            "MM: Memory Information =>\n"
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

        KernelHeap::MapEarlyHeap();
    }

    Pointer         KernelPhysicalAddress() { return s_KernelPhysicalAddress; }
    Pointer         KernelVirtualAddress() { return s_KernelVirtualAddress; }
    usize           GetHigherHalfOffset() { return s_HigherHalfOffset; }
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

    Ref<Region> AllocateKernelRegion(const usize bytes, PageAttributes flags)
    {
        auto  region    = VMM::AllocateKernelRegion(bytes, flags);
        usize pageCount = Math::DivRoundUp(bytes, PMM::PAGE_SIZE);
        auto  phys      = PMM::CallocatePages(pageCount);

        using VMM::Access;
        Access access = Access::eUser;
        if (flags & PageAttributes::eRead) access |= Access::eRead;
        if (flags & PageAttributes::eWrite) access |= Access::eWrite;
        if (flags & PageAttributes::eExecutable) access |= Access::eExecute;

        region->SetPhysicalBase(phys);
        region->SetAccessMode(access);

        auto pageMap = VMM::GetKernelPageMap();
        pageMap->MapRegion(region);

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

        auto        process = Process::Current();
        Ref<Region> region  = nullptr;

        if (process)
        {
            auto& addressSpace = process->AddressSpace();
            region             = addressSpace.Find(info.VirtualAddress());
        }

        if (region)
        {
            auto  fd   = region->FileDescriptor();
            usize size = fd && fd->File() ? fd->File()->Size() : region->Size();

            usize pageCount = Math::DivRoundUp(size, PMM::PAGE_SIZE);
            Pointer phys    = PMM::CallocatePages(pageCount);
            auto    virt    = region->VirtualBase();
            Assert(virt);

            auto pageMap = process->PageMap;
            if (phys)
            {
                auto flags = PageAttributes::eRW | PageAttributes::eUser
                           | PageAttributes::eWriteBack;
                region->SetPhysicalBase(phys);
                region->SetAttributes(flags);
                pageMap->MapRegion(region);

                if (fd)
                {
                    isize nread = TryAcquire(fd->Read(virt, size));
                    if (nread != static_cast<isize>(size))
                        LogError("MM: Failed to read the file descriptor");
                }
                return;
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
}; // namespace MM
