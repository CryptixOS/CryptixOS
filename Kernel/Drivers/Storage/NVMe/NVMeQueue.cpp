/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Drivers/Storage/NVMe/NVMeQueue.hpp>

namespace NVMe
{
    Queue::Queue(volatile u32* submitDoorbell, volatile u32* completeDoorbell,
                 u16 qid, u8 irq, u32 depth)
    {

        m_Submit           = new volatile Submission[depth];
        m_SubmitDoorbell   = submitDoorbell;
        m_SubmitHead       = 0;
        m_SubmitTail       = 0;
        m_Complete         = new volatile Completion[depth];
        m_CompleteDoorbell = completeDoorbell;
        m_CompleteVec      = 0;
        m_CompleteHead     = 0;
        m_CompletePhase    = 1;
        m_Depth            = depth;
        m_ID               = qid;
        m_CmdId            = 0;
        m_PhysRegPgs       = nullptr;
    }

    u16 Queue::AwaitSubmit(Submission* cmd)
    {
        u16 currentHead   = m_CompleteHead;
        u16 currentPhase  = m_CompletePhase;
        cmd->Identify.cid = m_CmdId++;
        Submit(cmd);
        u16 status = 0;

        while (true)
        {
            status = m_Complete[m_CompleteHead].Status;
            if ((status & 0x01) == currentPhase) break;
        }

        status >>= 1;
        AssertFmt(!status, "NVMe: Command error: {:#x}", status);

        m_CompleteHead++;
        if (m_CompleteHead == m_Depth)
        {
            m_CompleteHead  = 0;
            m_CompletePhase = !m_CompletePhase;
        }

        *(m_CompleteDoorbell) = currentHead;
        return status;
    }
    void Queue::Submit(Submission* cmd)
    {
        u16       currentTail = m_SubmitTail;

        uintptr_t dest = reinterpret_cast<uintptr_t>(&m_Submit[currentTail]);

        std::memcpy(reinterpret_cast<u8*>(dest), cmd, sizeof(Submission));
        currentTail++;

        if (currentTail == m_Depth) currentTail = 0;
        *(m_SubmitDoorbell) = currentTail;
        m_SubmitTail        = currentTail;
    }
}; // namespace NVMe
