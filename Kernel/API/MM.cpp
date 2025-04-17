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
    ErrorOr<intptr_t> SysMMap(Pointer addr, usize length, i32 prot, i32 flags,
                              i32 fdNum, off_t offset)
    {
        Process*               current   = Process::GetCurrent();
        std::optional<errno_t> errorCode = std::nullopt;

        using VirtualMemoryManager::Access;
        Access access = Access::eUser;
        if (prot & PROT_READ) access |= Access::eRead;
        if (prot & PROT_WRITE) access |= Access::eWrite;
        if (prot & PROT_EXEC) access |= Access::eExecute;

        if (length == 0) return Error(EINVAL);
        length             = Math::AlignUp(length, PMM::PAGE_SIZE);

        FileDescriptor* fd = nullptr;
        if (fdNum != -1) fd = current->GetFileHandle(fdNum);

        if (offset != 0) return Error(EINVAL);
        usize pageCount
            = Math::AlignUp(length, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

        auto&   addressSpace = current->GetAddressSpace();
        auto*   pageMap      = current->PageMap;
        Region* region       = nullptr;

        Pointer phys         = PMM::CallocatePages<uintptr_t>(pageCount);
        if (!phys) return Error(ENOMEM);

        if (flags & MAP_FIXED)
            region = addressSpace.AllocateFixed(addr, length);
        else if (flags & MAP_ANONYMOUS)
            region = addressSpace.AllocateRegion(addr, length);
        // TODO(v1tr10l7): File mapping
        else if (fdNum != -1)
        {
            if (!fd)
            {
                errorCode = EBADF;
                goto free_pages;
            }
            if (!region) region = addressSpace.AllocateRegion(addr, length);

            if (!fd->CanRead())
            {
                errorCode = EPERM;
                goto free_region;
            }
            pageMap->MapRegion(region);

            ErrorOr<isize> sizeOr;
            if (offset) sizeOr = fd->Seek(SEEK_SET, offset);
            if (!sizeOr)
            {
                errorCode = sizeOr.error();
                goto free_region;
            }

            sizeOr = fd->Read(region->GetVirtualBase(), length);
            if (!sizeOr)
            {
                errorCode = sizeOr.error();
                goto free_region;
            }

            return region->GetVirtualBase().Raw();
        }

        // TODO(v1tr10l7): sharring mappings
        if (flags & MAP_SHARED)
            ;
        if (!region) region = addressSpace.AllocateRegion(addr, length);

        // TODO(v1tr10l7): lowerhalf mappings
        // TODO(v1tr10l7): support for 2MiB and 1GiB pages
        if (!region) goto free_pages;

        region->SetPhysicalBase(phys);
        region->SetProt(access, prot);
        pageMap->MapRegion(region);

        return region->GetVirtualBase().Raw();

    free_region:
        addressSpace.Erase(region);
    free_pages:
        PMM::FreePages(phys, pageCount);
    [[maybe_unused]] fail:
        return Error(errorCode.value_or(ENOMEM));
    }
} // namespace API::MM

namespace Syscall::MM
{
    ErrorOr<intptr_t> SysMProtect(Arguments& args) { return 0; }
    ErrorOr<i32>      SysMUnMap(Arguments& args) { return 0; }

}; // namespace Syscall::MM
