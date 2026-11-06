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

    // Whether the PMM owns this region's backing pages. Device mappings (e.g.
    // the framebuffer) point at memory the PMM never allocated.
    static bool OwnsPhysicalMemory(const Region& region)
    {
        auto fd   = region.FileDescriptor();
        auto file = fd ? fd->File() : nullptr;
        return !(file && (file->IsCharDevice() || file->IsBlockDevice()));
    }

    // Removes [base, base + length) from the address space, like munmap():
    // regions entirely inside the range are dropped, regions overlapping it
    // are split and keep their parts outside the range. Translations are
    // removed before memory is freed, so no stale PTE can survive.
    // Also used by MAP_FIXED, which must replace whatever is mapped there
    // (mlibc's ld.so reserves a DSO's range with PROT_NONE, then maps each
    // segment over the reservation with MAP_FIXED).
    static void RemoveRange(Process& process, Pointer base, usize length)
    {
        auto&   addressSpace = process.AddressSpace();
        Pointer end          = base.Offset<Pointer>(length);

        // Collect first: the tree must not be modified while iterating it.
        Vector<::Ref<Region>> overlapping;
        for (const auto& [virt, region] : addressSpace)
            if (region->VirtualBase() < end && region->End() > base)
                overlapping.PushBack(region);

        for (auto& region : overlapping)
        {
            Pointer regionBase  = region->VirtualBase();
            Pointer regionEnd   = region->End();
            Pointer cutBase     = Max(regionBase.Raw(), base.Raw());
            Pointer cutEnd      = Min(regionEnd.Raw(), end.Raw());
            usize   cutOffset   = cutBase.Raw() - regionBase.Raw();
            usize   cutSize     = cutEnd.Raw() - cutBase.Raw();
            Pointer phys        = region->PhysicalBase();
            bool    ownsMemory  = OwnsPhysicalMemory(*region);
            auto    fd          = region->FileDescriptor();
            usize   fileOffset  = region->FileOffset();
            auto    access      = region->Access();

            // Unbacked regions were never mapped, so they have no PTEs.
            if (phys) process.PageMap->UnmapRange(cutBase, cutSize);
            addressSpace.Erase(regionBase);
            if (phys && ownsMemory)
                PMM::FreePages(phys.Offset<Pointer>(cutOffset),
                               cutSize / PMM::PAGE_SIZE);

            // Re-insert the parts that lie outside the removed range. Their
            // backing memory (if any) is the matching slice of the original.
            auto keep = [&](Pointer partBase, Pointer partEnd)
            {
                if (partBase >= partEnd) return;
                usize offset = partBase.Raw() - regionBase.Raw();
                auto  part   = CreateRef<Region>(
                    phys ? phys.Offset<Pointer>(offset) : Pointer(nullptr),
                    partBase, partEnd.Raw() - partBase.Raw(), fd);
                part->SetFileDescriptor(fd, fileOffset + offset);
                part->SetAccessMode(access);
                addressSpace.Insert(part);
            };
            keep(regionBase, cutBase);
            keep(cutEnd, regionEnd);
        }
    }

    // If a region strictly contains `at`, splits it into [base, at) and
    // [at, end). Both halves keep the access mode, file and (if already
    // backed) the matching slice of the physical memory.
    static void SplitAt(AddressSpace& addressSpace, Pointer at)
    {
        auto region = addressSpace.Find(at);
        if (!region || region->VirtualBase() == at) return;

        Pointer regionBase = region->VirtualBase();
        Pointer regionEnd  = region->End();
        Pointer phys       = region->PhysicalBase();
        auto    fd         = region->FileDescriptor();
        usize   fileOffset = region->FileOffset();
        auto    access     = region->Access();

        addressSpace.Erase(regionBase);
        auto make = [&](Pointer partBase, Pointer partEnd)
        {
            usize offset = partBase.Raw() - regionBase.Raw();
            auto  part   = CreateRef<Region>(
                phys ? phys.Offset<Pointer>(offset) : Pointer(nullptr),
                partBase, partEnd.Raw() - partBase.Raw(), fd);
            part->SetFileDescriptor(fd, fileOffset + offset);
            part->SetAccessMode(access);
            addressSpace.Insert(part);
        };
        make(regionBase, at);
        make(at, regionEnd);
    }

    ErrorOr<ipointer> MMap(Pointer addr, usize length, i32 prot, i32 flags,
                           i32 fdNum, off_t offset)
    {
        Process*          current   = Process::GetCurrent();
        Optional<errno_t> errorCode = NullOpt;
        // LogDebug(
        //     "MMap: addr: {:#x}, length: {:#x}, prot: {:#x}, flags: {:#x}, "
        //     "fdNum: {}, offset: {:#x}",
        //     addr.Raw(), length, prot, flags, fdNum, offset);

        if (addr.Raw() & ~Arch::VMM::GetAddressMask()) return Error(EFAULT);
        flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
        // NOTE(v1tr10l7): Currently we don't support mapping the files
        // if (!(flags & MAP_ANONYMOUS)) return Error(EFAULT);

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

        auto&       addressSpace = current->AddressSpace();
        Ref<Region> region       = nullptr;

        // TODO(v1tr10l7): File mapping
        if (fdNum != -1 && !(flags & MAP_ANONYMOUS))
        {
            if (!fd)
            {
                errorCode = EBADF;
                goto fail;
            }
            region    = addressSpace.AllocateRegion(length);
            auto virt = region->VirtualBase();

            if (!fd->CanRead())
            {
                errorCode = EPERM;
                goto free_region;
            }

            region->SetAccessMode(access);
            region->SetFileDescriptor(fd.Raw(), offset);
            auto file = fd->File();
            if (!file)
            {
                errorCode = EBADF;
                goto free_region;
            }
            if (auto res = file->MapRegion(region); !res)
            {
                errorCode = res.Error();
                goto free_region;
            }

            return virt.Raw();
        }
        if (offset)
        {
            errorCode = EINVAL;
            goto fail;
        }
        if (!(flags & MAP_ANONYMOUS))
        {
            errorCode = ENOSYS;
            goto fail;
        }

        if (flags & MAP_FIXED)
        {
            if (addr.Raw() & (PMM::PAGE_SIZE - 1))
            {
                errorCode = EINVAL;
                goto fail;
            }
            // MAP_FIXED replaces any existing mapping in the range.
            RemoveRange(*current, addr, Math::AlignUp(length, PMM::PAGE_SIZE));
            region = addressSpace.AllocateFixed(addr, length);
        }
        else if (flags & MAP_ANONYMOUS)
            region = addressSpace.AllocateRegion(length, pageSize);

        // TODO(v1tr10l7): sharring mappings
        if (flags & MAP_SHARED) {};

        if (!region) goto fail;
        region->SetAccessMode(access);

        Assert(addressSpace.Find(region->VirtualBase()) == region);
        return region->VirtualBase().Raw();

    free_region:
        addressSpace.Erase(region->VirtualBase());
    [[maybe_unused]] fail:
        return Error(errorCode.ValueOr(ENOMEM));
    }
    ErrorOr<isize> MProtect(Pointer virt, usize length, i32 prot)
    {
        if (virt.Raw() & (PMM::PAGE_SIZE - 1)) return Error(EINVAL);
        if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) return Error(EINVAL);
        length = Math::AlignUp(length, PMM::PAGE_SIZE);
        if (length == 0) return 0;

        Pointer end = virt.Offset<Pointer>(length);
        if (end < virt) return Error(EINVAL);

        auto  process      = Process::GetCurrent();
        auto& addressSpace = process->AddressSpace();

        // Linux fails with ENOMEM if any page in the range is unmapped.
        for (Pointer cursor = virt; cursor < end;)
        {
            auto region = addressSpace.Find(cursor);
            if (!region) return Error(ENOMEM);
            cursor = region->End();
        }

        // Make the range boundaries region boundaries, so only the requested
        // pages change protection.
        SplitAt(addressSpace, virt);
        SplitAt(addressSpace, end);

        auto access = Prot2AccessFlags(prot);
        for (Pointer cursor = virt; cursor < end;)
        {
            auto region = addressSpace.Find(cursor);
            region->SetAccessMode(access);
            // Backed regions are already mapped: update their PTEs. Unbacked
            // ones pick the new attributes up when they are demand-paged.
            if (region->PhysicalBase())
            {
                process->PageMap->ProtectRange(region->VirtualBase(),
                                               region->Size(),
                                               region->PageAttributes());
            }
            cursor = region->End();
        }
        return 0;
    }
    ErrorOr<isize> MUnMap(Pointer virt, usize length)
    {
        auto process = Process::GetCurrent();
        if (virt.Raw() & (PMM::PAGE_SIZE - 1)) return Error(EINVAL);
        if (length == 0) return Error(EINVAL);

        // Unmapping a range with nothing in it is not an error on Linux.
        RemoveRange(*process, virt, Math::AlignUp(length, PMM::PAGE_SIZE));
        return 0;
    }
} // namespace API::MM
