/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>
#include <Arch/CPU.hpp>

#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>
#include <Drivers/Storage/StorageDevicePartition.hpp>

#include <Memory/PMM.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace NVMe
{
    NameSpace::NameSpace(StringView name, u32 id, Controller* controller)
        : StorageDevice(name, MakeDevice(API::DeviceMajor::BLOCK_EXTENDED, 0))
        , m_ID(id)
        , m_Controller(controller)
    {
    }

    bool NameSpace::Initialize()
    {
        NameSpaceInfo* info = new NameSpaceInfo;
        if (!Identify(info)) return false;

        u64 formattedLba = info->FormattedLbaSize & 0x0f;
        u64 lbaShift     = info->LbaFormatUpper[formattedLba].DataSize;
        u64 maxLbas      = 1 << (m_Controller->GetMaxTransShift() - lbaShift);
        m_MaxPhysRPages  = (maxLbas * (1 << lbaShift)) / PMM::PAGE_SIZE;

        if (!m_Controller->CreateIoQueues(*this, m_IoQueue, m_ID)) return false;
        m_Cache            = new CachedBlock[512];
        m_LbaSize          = 1 << info->LbaFormatUpper[formattedLba].DataSize;
        m_CacheBlockSize   = m_LbaSize * 4;
        m_LbaCount         = info->TotalBlockCount;

        m_Stats.st_size    = info->TotalBlockCount * m_LbaSize;
        m_Stats.st_blocks  = info->TotalBlockCount;
        m_Stats.st_blksize = m_LbaSize;
        m_BlockSize        = m_LbaSize;
        m_Stats.st_rdev    = ID();
        m_Stats.st_mode    = 0666 | S_IFBLK;

        DeviceManager::RegisterBlockDevice(this);
        StringView path = fmt::format("/dev/{}", Name()).data();
        LogTrace("NVMe: Creating namespace node at `{}`", path);

        m_PartitionTable.Load(*this);
        usize i = 1;
        for (const auto& entry : m_PartitionTable)
        {
            StringView name = fmt::format("{}p{}", Name(), i).data();

            StorageDevicePartition* partition = new StorageDevicePartition(
                name, *this, entry.FirstBlock, entry.LastBlock,
                API::DeviceMajor::BLOCK_EXTENDED, i);

            DeviceManager::RegisterBlockDevice(partition);
            LogTrace("NVMe: Addining a device partition at `{}`",
                     fmt::format("/dev/{}", name.Raw()).data());

            ++i;
        }
        return true;
    }

    ErrorOr<isize> NameSpace::Read(void* dest, off_t offset, usize bytes)
    {
        ScopedLock guard(m_Lock);

        for (usize progress = 0; progress < bytes;)
        {
            u64 sector = (offset + progress) / m_CacheBlockSize;
            i32 slot   = FindBlock(sector);
            if (slot == -1) slot = CacheBlock(sector);
            if (slot == -1) return -1;

            u64   chunk = bytes - progress;
            usize off   = (offset + progress) % m_CacheBlockSize;
            if (chunk > m_CacheBlockSize - off) chunk = m_CacheBlockSize - off;

            Memory::Copy(reinterpret_cast<u8*>(dest) + progress,
                         &m_Cache[slot].Cache[off], chunk);
            progress += chunk;
        }

        return bytes;
    }
    ErrorOr<isize> NameSpace::Write(const void* src, off_t offset, usize bytes)
    {
        ScopedLock guard(m_Lock);

        for (usize progress = 0; progress < bytes;)
        {
            u64 sector = (offset + progress) / m_CacheBlockSize;
            i32 slot   = FindBlock(sector);
            if (slot == -1)
            {
                slot = CacheBlock(sector);
                if (slot == -1) return -1;
            }

            u64   chunk = bytes - progress;
            usize off   = (offset + progress) % m_CacheBlockSize;
            if (chunk > m_CacheBlockSize - off) chunk = m_CacheBlockSize - off;

            const u8* dest = reinterpret_cast<const u8*>(src) + progress;
            Memory::Copy(&m_Cache[slot].Cache[off], dest, chunk);
            m_Cache[slot].Status = CacheReady;

            i32 nwritten         = ReadWriteLba(m_Cache[slot].Cache,
                                                (m_CacheBlockSize / m_LbaSize)
                                                    * m_Cache[slot].Block,
                                                m_CacheBlockSize / m_LbaSize, true);
            if (nwritten == -1) return -1;
            progress += chunk;
        }

        return bytes;
    }

    ErrorOr<isize> NameSpace::Read(const UserBuffer& out, usize count,
                                   isize offset)
    {
        return Read(out.Raw(), offset, count);
    }
    ErrorOr<isize> NameSpace::Write(const UserBuffer& in, usize count,
                                    isize offset)
    {
        return NameSpace::Write(in.Raw(), offset, count);
    }

    bool NameSpace::Identify(NameSpaceInfo* nsid)
    {
        Submission cmd   = {};
        cmd.OpCode       = OpCode::ADMIN_IDENTIFY;
        cmd.NameSpaceID  = m_ID;
        cmd.Identify.Cns = 0;
        cmd.Prp1         = Pointer(nsid).FromHigherHalf<u64>();

        u16 status       = m_Controller->GetAdminQueue()->AwaitSubmit(&cmd);
        if (status) return false;

        return true;
    }

    isize NameSpace::ReadWriteLba(u8* dest, usize start, usize lbaCount,
                                  bool write)
    {
        /* ---------- Validate request ---------- */

        if (!dest) return -1;

        if (start >= m_LbaCount) return -1;

        if (lbaCount == 0) return 0;

        if (start + lbaCount > m_LbaCount) lbaCount = m_LbaCount - start;

        const usize totalBytes = lbaCount * m_LbaSize;

        /* ---------- PRP layout calculation ---------- */

        const usize pageSize   = PMM::PAGE_SIZE;
        const usize pageOffset
            = reinterpret_cast<uintptr_t>(dest) & (pageSize - 1);

        const usize firstPageBytes
            = Math::Min(pageSize - pageOffset, totalBytes);

        usize remainingBytes
            = totalBytes > firstPageBytes ? totalBytes - firstPageBytes : 0;

        const bool needsPrp2    = remainingBytes > 0;
        const bool needsPrpList = remainingBytes > pageSize;

        /* ---------- Allocate PRP list if needed ---------- */

        u32        cid          = m_IoQueue->GetCommandID();

        u64*       prpList      = nullptr;
        usize      prpCount     = 0;

        if (needsPrpList)
        {
            prpCount = (remainingBytes + pageSize - 1) / pageSize;

            if (prpCount > m_MaxPhysRPages)
            {
                LogError("NVMe: PRP list overflow ({} > {})", prpCount,
                         m_MaxPhysRPages);
                return -1;
            }

            prpList = &m_IoQueue->GetPhysRegPages()[cid * m_MaxPhysRPages];

            uintptr_t pageBase
                = reinterpret_cast<uintptr_t>(dest) & ~(pageSize - 1);

            for (usize i = 0; i < prpCount; ++i)
                prpList[i] = Pointer(pageBase + pageSize * (i + 1))
                                 .FromHigherHalf<u64>();
        }

        /* ---------- Build command ---------- */

        Submission cmd{};
        cmd.OpCode         = write ? OpCode::IO_WRITE : OpCode::IO_READ;
        cmd.Flags          = 0;
        cmd.NameSpaceID    = m_ID;

        cmd.ReadWrite.SLba = start;
        cmd.ReadWrite.Len  = lbaCount - 1;

        cmd.Prp1           = Pointer(dest).FromHigherHalf<u64>();

        if (needsPrpList) { cmd.Prp2 = Pointer(prpList).FromHigherHalf<u64>(); }
        else if (needsPrp2)
        {
            uintptr_t secondPage
                = (reinterpret_cast<uintptr_t>(dest) & ~(pageSize - 1))
                + pageSize;

            cmd.Prp2 = Pointer(secondPage).FromHigherHalf<u64>();
        }
        else {
            cmd.Prp2 = 0;
        }

        /* ---------- Submit ---------- */

        u16 status = m_IoQueue->AwaitSubmit(&cmd);

        if (status)
        {
            LogError("NVMe: I/O failed (status = {:#x})", status);
            return -1;
        }

        return lbaCount;
    }

    isize NameSpace::FindBlock(u64 block)
    {
        for (usize i = 0; i < 512; i++)
            if (m_Cache[i].Block == block && m_Cache[i].Status) return i;

        return -1;
    }
    isize NameSpace::CacheBlock(u64 block)
    {
        i32 target = 0;

        for (target = 0; target < 512; target++)
            if (!m_Cache[target].Status)
            {
                m_Cache[target].Cache = new u8[m_CacheBlockSize];
                goto write;
            }

        if (m_Overwritten == 512)
        {
            m_Overwritten = 0;
            target        = m_Overwritten;
        }
        else target = m_Overwritten++;

    write:
        auto lba = ReadWriteLba(m_Cache[target].Cache,
                                (m_CacheBlockSize / m_LbaSize) * block,
                                m_CacheBlockSize / m_LbaSize, 0);
        if (lba == -1) return lba;

        m_Cache[target].Block  = block;
        m_Cache[target].Status = CacheReady;

        return target;
    }
}; // namespace NVMe
