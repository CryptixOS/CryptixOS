/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>

#include <Drivers/DeviceManager.hpp>
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>
#include <Drivers/Storage/StorageDevicePartition.hpp>

#include <Memory/PMM.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace NVMe
{
    NameSpace::NameSpace(u32 id, Controller* controller)
        : StorageDevice(259, 0)
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
        m_Stats.st_rdev    = 0;
        m_Stats.st_mode    = 0666 | S_IFBLK;

        DevTmpFs::RegisterDevice(this);

        StringView path
            = fmt::format("/dev/{}n{}", m_Controller->GetName(), m_ID).data();
        LogTrace("NVMe: Creating device at '{}'", path);
        VFS::MkNod(nullptr, path, m_Stats.st_mode, GetID());
        DeviceManager::RegisterBlockDevice(this);
        // TODO(v1tr10l7): enumerate partitions

        m_PartitionTable.Load(*this);

        for (usize i = 0; i < m_IoQueue->GetDepth(); i++)
        {
            break;
            auto handler = InterruptManager::AllocateHandler();
            handler->Reserve();
            /*
            if (!MsiXSet(CPU::GetCurrent()->LapicID,
                         handler->GetInterruptVector(), -1))
            {
                if (i == 0)
                    LogError(
                        "Could not install any irq handlers for io queues");
                break;
            }*/
        }

        usize i = 1;
        for (const auto& entry : m_PartitionTable)
        {
            StorageDevicePartition* partition = new StorageDevicePartition(
                *this, entry.FirstBlock, entry.LastBlock, 292, i);
            DevTmpFs::RegisterDevice(partition);
            DeviceManager::RegisterBlockDevice(partition);

            StringView partitionPath
                = fmt::format("/dev/{}n{}p{}", m_Controller->GetName(), m_ID, i)
                      .data();
            VFS::MkNod(nullptr, partitionPath, m_Stats.st_mode,
                       partition->GetID());

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

            std::memcpy(reinterpret_cast<u8*>(dest) + progress,
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
            std::memcpy(&m_Cache[slot].Cache[off], dest, chunk);
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

    StringView NameSpace::GetName() const noexcept
    {
        auto name = fmt::format("{}n{}", m_Controller->GetName(), m_ID);
        return StringView(name.data(), name.size());
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

    isize NameSpace::ReadWriteLba(u8* dest, usize start, usize bytes, u8 write)
    {
        if (start + bytes >= m_LbaCount) bytes -= (start + bytes) - m_LbaCount;

        usize pageOff          = u64(dest) & (PMM::PAGE_SIZE - 1);
        bool  shouldUsePrp     = false;
        bool  shouldUsePrpList = false;

        u32   cid              = m_IoQueue->GetCommandID();

        if ((bytes * m_LbaSize) > PMM::PAGE_SIZE)
        {
            if ((bytes * m_LbaSize) > (PMM::PAGE_SIZE * 2))
            {
                usize prpcount = ((bytes - 1) * m_LbaSize) / PMM::PAGE_SIZE;
                AssertFmt(!(prpcount > m_MaxPhysRPages),
                          "NVMe: Exceeded physical region pages, prpcount => "
                          "{:#x}, bytes => {:#x}",
                          prpcount, bytes);
                for (usize i = 0; i < prpcount; i++)
                {
                    m_IoQueue->GetPhysRegPages()[i + cid * m_MaxPhysRPages]
                        = (Pointer(dest - pageOff).FromHigherHalf<u64>()
                           + PMM::PAGE_SIZE + i * PMM::PAGE_SIZE);
                }
                shouldUsePrp     = false;
                shouldUsePrpList = true;
            }
            else shouldUsePrp = true;
        }

        Submission cmd        = {};
        cmd.OpCode            = write ? OpCode::IO_WRITE : OpCode::IO_READ;
        cmd.Flags             = 0;
        cmd.NameSpaceID       = m_ID;
        cmd.ReadWrite.Control = 0;
        cmd.ReadWrite.Dsmgmt  = 0;
        cmd.ReadWrite.Ref     = 0;
        cmd.ReadWrite.AppTag  = 0;
        cmd.ReadWrite.AppMask = 0;
        cmd.Metadata          = 0;
        cmd.ReadWrite.SLba    = start;
        cmd.ReadWrite.Len     = bytes - 1;
        if (shouldUsePrpList)
        {
            cmd.Prp1 = Pointer(dest).FromHigherHalf<u64>();
            cmd.Prp2
                = Pointer(&m_IoQueue->GetPhysRegPages()[cid * m_MaxPhysRPages])
                      .FromHigherHalf<u64>();
        }
        else if (shouldUsePrp)
            cmd.Prp2 = Pointer(dest)
                           .Offset<Pointer>(PMM::PAGE_SIZE)
                           .FromHigherHalf<u64>();

        else cmd.Prp1 = Pointer(dest).FromHigherHalf<u64>();

        u16 status = m_IoQueue->AwaitSubmit(&cmd);
        AssertFmt(!status, "nvme: failed to read/write with status {:#x}\n",
                  status);
        return 0;
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
