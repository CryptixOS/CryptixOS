/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/MM.hpp>
#include <API/Posix/sys/mman.h>
#include <API/UnixTypes.hpp>

#include <Arch/CPU.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Math.hpp>

namespace API::MM
{
    ErrorOr<intptr_t> SysMMap(Pointer addr, usize len, i32 prot, i32 flags,
                              i32 fdNum, off_t offset)
    {
        Process*        current = Process::GetCurrent();
        FileDescriptor* fd      = nullptr;

        // LogInfo(
        //     "addr: {:#x}, len: {:#x}, prot: {:#x}, flags: {:#x}, fdNum:
        //     {:#x}, " "offset: {:#x}", addr.Raw(), len, prot, flags, fdNum,
        //     offset);

        if (fdNum != -1 || !(flags & MAP_ANONYMOUS))
        {
            fd = current->GetFileHandle(fdNum);
            if (!fd) return Error(EBADF);
            // TODO(v1tr10l7): map fd
            return Error(ENOSYS);
        }
        if (offset != 0) return Error(EINVAL);

        if (len == 0) return Error(EINVAL);
        len = Math::AlignUp(len, PMM::PAGE_SIZE);

        if (!(flags & MAP_ANONYMOUS)) return Error(ENOSYS);
        uintptr_t virt = addr;
        if (!(flags & MAP_FIXED)) virt = VMM::AllocateSpace(len, 0, true);

        usize pageCount = Math::AlignUp(len, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;
        uintptr_t phys  = PMM::CallocatePages<uintptr_t>(pageCount);
        if (!phys) return Error(ENOMEM);

        PageAttributes attributes = PageAttributes::eUser;
        if (prot & PROT_READ) attributes |= PageAttributes::eRead;
        if (prot & PROT_WRITE) attributes |= PageAttributes::eWrite;
        if (prot & PROT_EXEC) attributes |= PageAttributes::eExecutable;

        current->PageMap->MapRange(virt, phys, len,
                                   attributes | PageAttributes::eWriteBack);

        ScopedLock guard(current->m_Lock);
        current->m_AddressSpace.push_back({phys, virt, len, prot});
        DebugSyscall("MMAP: virt: {:#x}", virt);
        return virt;
    }
} // namespace API::MM

namespace Syscall::MM
{
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
