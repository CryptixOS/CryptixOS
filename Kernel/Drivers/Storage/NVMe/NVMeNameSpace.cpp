/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>

#include <Memory/PMM.hpp>

namespace NVMe
{
    NameSpace::NameSpace(u32 id, Controller* controller)
        : m_ID(id)
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

        // TODO(v1tr10l7): create io queues;

        return true;
    }

    bool NameSpace::Identify(NameSpaceInfo* nsid)
    {
        Submission cmd      = {};
        cmd.Identify.opcode = 0x06;
        cmd.Identify.nsid   = m_ID;
        cmd.Identify.cns    = 0;
        cmd.Identify.prp1   = Pointer(nsid).FromHigherHalf<u64>();

        u16 status          = m_Controller->GetAdminQueue()->AwaitSubmit(&cmd);
        if (status) return false;

        return true;
    }
}; // namespace NVMe
