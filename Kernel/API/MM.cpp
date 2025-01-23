/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/UnixTypes.hpp>

#include <Arch/CPU.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Math.hpp>

namespace Syscall::MM
{
    ErrorOr<intptr_t> SysMMap(Arguments& args)
    {
        uintptr_t  addr    = args.Args[0];
        usize      len     = args.Args[1];
        usize      prot    = args.Args[2];
        usize      flags   = args.Args[3];
        i32        fd      = args.Args[4];
        off_t      offset  = args.Args[5];

        Process*   process = CPU::GetCurrentThread()->parent;
        ScopedLock guard(process->m_Lock);
        DebugSyscall(
            "MMAP: hint: {:#x}, size: {}, prot: {}, flags: {}, fd: {}, offset: "
            "{}",
            addr, len, prot, flags, fd, offset);

        if (flags & MAP_ANONYMOUS)
        {
            if (offset != 0)
            {
                LogError("MMAP: Failed, offset != 0 with MAP_ANONYMOUS");
                return_err(MAP_FAILED, EINVAL);
            }

            usize pageCount
                = Math::AlignUp(len, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;

            auto virt = addr;

            if (virt == 0) virt = VMM::AllocateSpace(len, 0, true);
            uintptr_t phys = PMM::CallocatePages<uintptr_t>(pageCount);
            process->PageMap->MapRange(virt, phys, len,
                                       PageAttributes::eRWXU
                                           | PageAttributes::eWriteBack);

            process->m_AddressSpace.push_back({phys, virt, len});
            DebugSyscall("MMAP: virt: {:#x}", virt);
            return virt;
        }

        CtosUnused(addr);
        CtosUnused(prot);
        CtosUnused(fd);

        LogError("MMAP: Failed, only MAP_ANONYMOUS is implemented");
        return_err(MAP_FAILED, EINVAL);
    }
    ErrorOr<i32> SysMUnMap(Arguments& args)
    {
        Pointer    addr    = args.Get<uintptr_t>(0);
        usize      length  = args.Get<usize>(1);

        Process*   current = Process::GetCurrent();
        ScopedLock guard(current->m_Lock);
        current->PageMap->UnmapRange(addr, length);

        Pointer                            phys = 0;
        std::vector<VMM::Region>::iterator it = current->m_AddressSpace.begin();
        for (; it < current->m_AddressSpace.end(); it++)
        {
            if (it->GetVirtualBase() == addr)
            {
                phys = it->GetPhysicalBase();
                break;
            }
        }

        if (!phys || it == current->m_AddressSpace.end()) return Error(EFAULT);
        usize pageCount
            = Math::AlignUp(length, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;
        PMM::FreePages(phys.ToHigherHalf<uintptr_t>(), pageCount);
        return 0;
    }

}; // namespace Syscall::MM
