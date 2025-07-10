/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/Posix/sys/mman.h>
#include <API/Posix/unistd.h>
#include <API/UnixTypes.hpp>

#include <Arch/CPU.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Utility/Math.hpp>

namespace API::MM
{
    using VMM::Access;
    static Access Prot2AccessFlags(isize prot)
    {
        Access access = Access::eUser;

        if (prot & PROT_READ) access |= Access::eRead;
        if (prot & PROT_WRITE) access |= Access::eWrite;
        if (prot & PROT_EXEC) access |= Access::eExecute;

        return access;
    }

    ErrorOr<intptr_t> MMap(Pointer addr, usize length, i32 prot, i32 flags,
                           i32 fdNum, off_t offset)
    {
        Process*          current   = Process::GetCurrent();
        Optional<errno_t> errorCode = NullOpt;

        if (addr.Raw() & ~Arch::VMM::GetAddressMask()) return MAP_FAILED;
        flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
        if (!(flags & MAP_ANONYMOUS)) return MAP_FAILED;

        // TODO(v1tr10l7): Lazy mapping
        using VMM::Access;
        Access access = Access::eUser;
        if (prot & PROT_READ) access |= Access::eRead;
        if (prot & PROT_WRITE) access |= Access::eWrite;
        if (prot & PROT_EXEC) access |= Access::eExecute;

        if (length == 0) return Error(EINVAL);
        length         = Math::AlignUp(length, PMM::PAGE_SIZE);
        usize pageSize = PMM::PAGE_SIZE;
        if (flags & MAP_HUGE_2MB) pageSize = 2_mib;
        else if (flags & MAP_HUGE_1GB) pageSize = 1_gib;

        Ref<FileDescriptor> fd = nullptr;
        if (fdNum != -1) fd = current->GetFileHandle(fdNum);

        if (offset != 0) return Error(EINVAL);
        usize pageCount
            = Math::AlignUp(length, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

        auto&       addressSpace = current->AddressSpace();
        auto*       pageMap      = current->PageMap;
        Ref<Region> region       = nullptr;

        Pointer     phys         = PMM::CallocatePages<uintptr_t>(pageCount);
        if (!phys) return Error(ENOMEM);

        // TODO(v1tr10l7): File mapping
        if (fdNum != -1 && !(flags & MAP_ANONYMOUS))
        {
            if (!fd)
            {
                errorCode = EBADF;
                goto free_pages;
            }
            region = addressSpace.AllocateRegion(length);
            region->SetPhysicalBase(phys);
            auto virt = region->VirtualBase();

            if (!fd->CanRead())
            {
                errorCode = EPERM;
                goto free_region;
            }

            region->SetAccessMode(access);
            pageMap->MapRegion(region, pageSize);

            ErrorOr<isize> sizeOr;
            if (offset) sizeOr = fd->Seek(SEEK_SET, offset);
            if (!sizeOr)
            {
                errorCode = sizeOr.error();
                goto free_region;
            }

            sizeOr = fd->Read(virt, length);
            if (!sizeOr)
            {
                errorCode = sizeOr.error();
                goto free_region;
            }

            return virt.Raw();
        }
        if (offset)
        {
            errorCode = EINVAL;
            goto free_pages;
        }
        if (!(flags & MAP_ANONYMOUS))
        {
            errorCode = ENOSYS;
            goto free_pages;
        }

        if (flags & MAP_FIXED)
            region = addressSpace.AllocateFixed(addr, length);
        else if (flags & MAP_ANONYMOUS)
            region = addressSpace.AllocateRegion(length, pageSize);

        // TODO(v1tr10l7): sharring mappings
        if (flags & MAP_SHARED)
            ;

        if (!region) goto free_pages;
        region->SetPhysicalBase(phys);
        region->SetAccessMode(access);
        pageMap->MapRegion(region, pageSize);

        return region->VirtualBase().Raw();

    free_region:
        addressSpace.Erase(region->VirtualBase());
    free_pages:
        PMM::FreePages(phys, pageCount);
    [[maybe_unused]] fail:
        return Error(errorCode.ValueOr(ENOMEM));
    }
    ErrorOr<isize> MProtect(Pointer virt, usize length, i32 prot)
    {
        if (virt & ~PMM::PAGE_SIZE) return Error(EINVAL);
        length = Math::AlignUp(length, PMM::PAGE_SIZE);
        if (length == 0) return Error(EINVAL);

        auto regionEnd = virt.Offset<Pointer>(length);
        if (regionEnd < virt) return Error(EINVAL);
        else if (virt == regionEnd) return 0;
        if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) return Error(EINVAL);

        auto  process      = Process::GetCurrent();
        auto& addressSpace = process->AddressSpace();
        auto  region       = addressSpace.Find(virt);
        if (!region) return Error(EFAULT);

        auto accessFlags = Prot2AccessFlags(prot);
        region->SetAccessMode(accessFlags);

        process->PageMap->RemapRegion(region);
        return 0;
    }
    ErrorOr<isize> MUnMap(Pointer virt, usize length)
    {
        auto  process      = Process::GetCurrent();
        auto& addressSpace = process->AddressSpace();
        if (!addressSpace.Contains(virt)) return Error(EINVAL);

        auto        region    = addressSpace[virt];

        const auto  phys      = region->PhysicalBase();
        const usize pageCount = region->Size() / PMM::PAGE_SIZE;
        PMM::FreePages(phys, pageCount);

        addressSpace.Erase(virt);
        return 0;
    }
} // namespace API::MM
