/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
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
#include <API/UnixTypes.hpp>
#include <API/Posix/unistd.h>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
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

module Cryptix:Impl.API.MM;

import :API.MM;
import :Arch.CPU;
import :Memory.PMM;
import :Memory.VMM;
import :Scheduler.Process;
import :Scheduler.Thread;

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
            region = addressSpace.AllocateFixed(addr, length);
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
        return 0;
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

        process->PageMap->ProtectRange(virt, length, region->PageAttributes());
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
