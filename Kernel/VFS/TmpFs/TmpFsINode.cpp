/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "TmpFsINode.hpp"

#include "Memory/KernelHeap.hpp"
#include "Utility/Math.hpp"
#include "VFS/TmpFs/TmpFs.hpp"

isize TmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    std::unique_lock guard(lock);

    usize            count = bytes;
    if (off_t(offset + bytes) > stats.st_size)
        count = bytes - ((offset + bytes) - stats.st_size);

    Assert(buffer);
    std::memcpy(buffer, reinterpret_cast<uint8_t*>(data) + offset, count);
    return count;
}
isize TmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    std::unique_lock guard(lock);

    if (offset + bytes > capacity)
    {
        usize newCapacity = capacity;
        while (offset + bytes >= newCapacity) newCapacity *= 2;

        auto tfs = reinterpret_cast<TmpFs*>(filesystem);
        if (tfs->currentSize + (newCapacity - capacity) > tfs->maxSize)
        {
            errno = ENOSPC;
            return -1;
        }

        data = static_cast<uint8_t*>(KernelHeap::Reallocate(data, newCapacity));
        if (!data) return -1;

        capacity = newCapacity;
    }

    memcpy(data + offset, buffer, bytes);

    if (off_t(offset + bytes) >= stats.st_size)
    {
        stats.st_size   = off_t(offset + bytes);
        stats.st_blocks = Math::DivRoundUp(stats.st_size, stats.st_blksize);
    }

    return bytes;
}
