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

#include <Prism/Math.hpp>

namespace API::MM
{
    inline PageAttributes Prot2PageAttributes(i32 prot)
    {
        PageAttributes attribs = PageAttributes::eUser;
        if (prot & PROT_READ) attribs |= PageAttributes::eRead;
        if (prot & PROT_WRITE) attribs |= PageAttributes::eWrite;
        if (prot & PROT_EXEC) attribs |= PageAttributes::eExecutable;

        return attribs;
    }

    ErrorOr<intptr_t> SysMMap(Pointer addr, usize len, i32 prot, i32 flags,
                              i32 fdNum, off_t offset)
    {
        Process*        current = Process::GetCurrent();
        FileDescriptor* fd      = nullptr;

        // LogInfo(
        //     "addr: {:#x}, len: {:#x}, prot: {:#x}, flags: {:#x}, fdNum:
        //     {:#x}, " "offset: {:#x}", addr.Raw(), len, prot, flags, fdNum,
        //     offset);

        if (len == 0) return Error(EINVAL);
        len = Math::AlignUp(len, PMM::PAGE_SIZE);
        if (fdNum != -1 || !(flags & MAP_ANONYMOUS))
        {
            fd = current->GetFileHandle(fdNum);
            if (!fd) return Error(EBADF);

            usize pageCount
                = Math::AlignUp(len, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;
            uintptr_t phys = PMM::CallocatePages<u64>(pageCount);
            uintptr_t virt = VMM::AllocateSpace(len, 0);

            if (!fd->CanRead()) return Error(EPERM);
            current->PageMap->MapRange(virt, phys, len,
                                       Prot2PageAttributes(prot));

            ErrorOr<isize> sizeOrError;
            if (offset) sizeOrError = fd->Seek(SEEK_SET, offset);
            if (!sizeOrError) return sizeOrError.error();

            sizeOrError = fd->Read(reinterpret_cast<void*>(virt), len);
            if (!sizeOrError) return sizeOrError.error();

            ScopedLock guard(current->m_Lock);
            current->m_AddressSpace.push_back({phys, virt, len, prot, fd});

            return virt;
        }
        if (offset != 0) return Error(EINVAL);

        if (!(flags & MAP_ANONYMOUS)) return Error(ENOSYS);
        uintptr_t virt = addr;
        if (!(flags & MAP_FIXED)) virt = VMM::AllocateSpace(len, 0, true);

        usize pageCount = Math::AlignUp(len, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;
        uintptr_t phys  = PMM::CallocatePages<uintptr_t>(pageCount);
        if (!phys) return Error(ENOMEM);

        current->PageMap->MapRange(virt, phys, len,
                                   Prot2PageAttributes(prot)
                                       | PageAttributes::eWriteBack);

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
        Pointer                            addr    = args.Get<uintptr_t>(0);
        usize                              length  = args.Get<usize>(1);

        Process*                           current = Process::GetCurrent();
        ScopedLock                         guard(current->m_Lock);

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
        FileDescriptor* fd = it->GetFileDescriptor();
        if (fd)
        {
            auto errorOrBytes = fd->Write(addr, length);
            if (!errorOrBytes) return errorOrBytes.error();
        }

        current->m_AddressSpace.erase(it);

        current->PageMap->UnmapRange(addr, length);
        usize pageCount
            = Math::AlignUp(length, PMM::PAGE_SIZE) / PMM::PAGE_SIZE;
        PMM::FreePages(phys.ToHigherHalf<uintptr_t>(), pageCount);
        return 0;
    }

}; // namespace Syscall::MM
