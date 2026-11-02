/*
 * Created by v1tr10l7 on 28.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
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
#include <Prism/Memory/Scope.hpp>

export module Cryptix:Drivers.Storage.StorageDevicePartition;

export import :Drivers.Storage.StorageDevice;

export class StorageDevicePartition : public BlockDevice
{
  public:
    StorageDevicePartition(StringView name, StorageDevice& device,
                           u64 firstBlock, u64 lastBlock, u16 majorID,
                           u16 minorID);

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        // if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Read(
            dest, m_Device.Stats().st_blksize * m_FirstBlock + offset, bytes);
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        // if (m_FirstBlock + offset >= m_LastBlock) return_err(-1, ENODEV);
        return m_Device.Write(
            src, m_Device.Stats().st_blksize * m_FirstBlock + offset, bytes);
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return Read(out.Raw(), offset, count);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Write(in.Raw(), offset, count);
    }

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
    {
        return m_Device.IoCtl(request, argp);
    }

  private:
    StorageDevice&  m_Device;
    u64             m_FirstBlock;
    CTOS_UNUSED u64 m_LastBlock;
};
