/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>

#include <Memory/PMM.hpp>
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace NVMe
{
    NameSpace::NameSpace(u32 id, Controller* controller)
        : ::Device(DriverType(259), DeviceType(0))
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

        if (!m_Controller->CreateIoQueues(*this, m_IoQueues, m_ID))
            return false;
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

        std::string_view path
            = std::format("/dev/{}n{}", m_Controller->GetName(), m_ID);
        VFS::MkNod(VFS::GetRootNode(), path, m_Stats.st_mode, GetID());
        // TODO(v1tr10l7): enumerate partitions

        return true;
    }

    std::string_view NameSpace::GetName() const noexcept
    {
        return std::format("{}n{}", m_Controller->GetName(), m_ID);
    }

    bool NameSpace::Identify(NameSpaceInfo* nsid)
    {
        Submission cmd           = {};
        cmd.Identify.OpCode      = OpCode::ADMIN_IDENTIFY;
        cmd.Identify.NameSpaceID = m_ID;
        cmd.Identify.Cns         = 0;
        cmd.Identify.Prp1        = Pointer(nsid).FromHigherHalf<u64>();

        u16 status = m_Controller->GetAdminQueue()->AwaitSubmit(&cmd);
        if (status) return false;

        return true;
    }
}; // namespace NVMe
