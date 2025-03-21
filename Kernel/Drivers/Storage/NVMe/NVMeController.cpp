/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeQueue.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace NVMe
{
    std::atomic<usize> Controller::s_ControllerCount = 0;

    Controller::Controller(const PCI::DeviceAddress& address)
        : PCI::Device(address, static_cast<DriverType>(241),
                      static_cast<DeviceType>(0))
        , m_Index(s_ControllerCount++)
    {
        if (m_Index == 0) DevTmpFs::RegisterDevice(this);

        m_Name += std::to_string(m_Index);
        // VFS::MkNod(VFS::GetRootNode(), std::format("/dev/{}", GetName()),
        // 0666,
        //          GetID());

        LogTrace("NVMe{}: Initializing...", m_Index);
        if (!Initialize())
        {
            LogError("NVMe{}: Failed to initialize", m_Index);
            return;
        }

        std::string_view path = std::format("/dev/{}", GetName());
        VFS::MkNod(VFS::GetRootNode(), path, 0666, GetID());
    }

    bool Controller::Initialize()
    {
        EnableMemorySpace();
        EnableBusMastering();
        PCI::Bar bar = GetBar(0);
        if (!bar)
        {
            LogError("NVMe: Failed to acquire BAR0!");
            return false;
        }

        LogInfo("NVMe{}: bar0 = {{ Base: {:#x}, Size: {:#x}, IsMMIO: {} }}",
                m_Index, bar.Address.Raw<u64>(), bar.Size, bar.IsMMIO);

        usize bar1  = PCI::Device::Read<u32>(PCI::RegisterOffset::eBar1);

        m_CrAddress = (bar.Address.Raw<u64>() & -8ull) | bar1 << 32;
        m_Register  = m_CrAddress.As<volatile ControllerRegister>();

        AssertMsg(bar.IsMMIO, "PCI bar is not memory mapped!");
        Assert((ReadAt(0x10, 4) & 0b111) == 0b100);

        uintptr_t addr
            = Math::AlignDown(m_CrAddress.Raw<u64>(), PMM::PAGE_SIZE);
        usize len = Math::AlignUp(bar.Size, PMM::PAGE_SIZE);
        Assert(VMM::GetKernelPageMap()->MapRange(addr, addr, len,
                                                 PageAttributes::eRW));

        LogTrace("NVMe: Successfully mapped bar0");

        auto nullOrError = Disable();
        if (!nullOrError) return nullOrError.error();

        m_DoorbellStride = (m_Register->Capabilities >> 32) & 0xf;
        m_QueueSlots     = std::min(m_Register->Capabilities & 0xffff,
                                    (m_Register->Capabilities >> 16) & 0xffff);
        u32 queueId      = 0;

        LogTrace("NVMe: Creating admin queue...");

        u64 doorbell = u64(m_CrAddress.Offset<u64>(PMM::PAGE_SIZE)
                           + ((2 * id) * (4 << m_DoorbellStride)));
        VMM::GetKernelPageMap()->MapRange(
            doorbell, doorbell, PMM::PAGE_SIZE * 4, PageAttributes::eRW);

        m_AdminQueue
            = new Queue(m_CrAddress, queueId, m_DoorbellStride, m_QueueSlots);

        u32 aqa = m_QueueSlots - 1;
        aqa |= aqa << 16;
        aqa |= aqa << 16;
        m_Register->AdminQueueAttributes = aqa;
        auto cc                          = m_Register->Configuration;
        cc                               = (6 << 16) | (4 << 20) | (1 << 0);

        volatile uintptr_t asq
            = reinterpret_cast<volatile uintptr_t>(m_AdminQueue->GetSubmit());
        volatile uintptr_t acq
            = reinterpret_cast<volatile uintptr_t>(m_AdminQueue->GetComplete());

        m_Register->AdminSubmissionQueue
            = IsHigherHalfAddress(asq) ? asq - BootInfo::GetHHDMOffset() : asq;
        m_Register->AdminCompletionQueue
            = IsHigherHalfAddress(acq) ? acq - BootInfo::GetHHDMOffset() : acq;
        m_Register->Configuration = cc;

        while (true)
        {
            volatile u32 status = m_Register->Status;
            if (status & Bit(0)) break;
            LogWarn("NVMe: Failed to CreateNode Admin Queues!");
            return false;
        }

        ControllerInfo* info = new ControllerInfo;
        AssertFmt(!this->Identify(info),
                  "NVMe{}: Failed to identify the controller", m_Index);

        usize namespaceCount = info->NamespaceCount;
        LogInfo("NVMe: NameSpace Count: {}", namespaceCount);
        LogInfo("Submission Entry size: {:#x}, {:#x}",
                info->SubmissionQueueEntrySize, sizeof(Submission));
        LogInfo("Completion Entry size: {:#x}, {:#x}",
                info->CompletionQueueEntrySize, sizeof(Completion));
        delete info;

        LogInfo("NVMe: Controller #{} initialized successfully", m_Index);

        return DetectNameSpaces(namespaceCount);
    }
    void          Controller::Shutdown() {}

    void          Controller::Reset() {}

    ErrorOr<void> Controller::Disable()
    {
        auto cc = m_Register->Configuration;
        if (cc & Bit(0))
        {
            cc &= ~Bit(0);
            m_Register->Configuration = cc;
        }

        return WaitReady();
    }
    ErrorOr<void> Controller::Enable()
    {
        auto cc = m_Register->Configuration;
        if (!(cc & Bit(0)))
        {
            cc |= Bit(0);
            m_Register->Configuration = cc;
        }

        return WaitReady();
    }
    ErrorOr<void> Controller::WaitReady()
    {
        while (m_Register->Status & Bit(0)) Arch::Pause();
        return {};
    }

    bool Controller::CreateIoQueues(NameSpace& ns, Queue*& ioQueue, u32 id)
    {
        ioQueue
            = new Queue(m_CrAddress, ns, id, m_DoorbellStride, m_QueueSlots);
        Submission cmd1 = {};
        cmd1.OpCode     = OpCode::ADMIN_CREATE_CQ;
        cmd1.Prp1 = Pointer(u64(ioQueue->GetComplete())).FromHigherHalf<u64>();
        cmd1.CreateCompletionQueue.CompleteQueueID    = id;
        cmd1.CreateCompletionQueue.Size               = m_QueueSlots - 1;
        cmd1.CreateCompletionQueue.CompleteQueueFlags = Bit(0);
        cmd1.CreateCompletionQueue.IrqVec             = 0;
        u16 status = m_AdminQueue->AwaitSubmit(&cmd1);
        if (status)
        {
            delete ioQueue;
            return false;
        }

        Submission cmd2 = {};
        cmd2.OpCode     = OpCode::ADMIN_CREATE_SQ;
        cmd2.Prp1 = Pointer(u64(ioQueue->GetSubmit())).FromHigherHalf<u64>();
        cmd2.CreateSubmissionQueue.SubmitQueueID    = id;
        cmd2.CreateSubmissionQueue.CompleteQueueID  = id;
        cmd2.CreateSubmissionQueue.Size             = m_QueueSlots - 1;
        cmd2.CreateSubmissionQueue.SubmitQueueFlags = Bit(0) | (2 << 1);
        status = m_AdminQueue->AwaitSubmit(&cmd2);
        if (status)
        {
            delete ioQueue;
            return false;
        }

        return true;
    }

    i32 Controller::Identify(ControllerInfo* info)
    {
        LogDebug("sizeof(Submission) = {}", sizeof(Submission));
        LogDebug("sizeof(Completion) = {}", sizeof(Completion));

        u64        len   = sizeof(ControllerInfo);
        Submission cmd   = {};
        cmd.OpCode       = OpCode::ADMIN_IDENTIFY;
        cmd.NameSpaceID  = 0;
        cmd.Identify.Cns = 1;
        cmd.Prp1         = FromHigherHalfAddress<uintptr_t>(info);
        i32 off          = u64(info) & (PMM::PAGE_SIZE - 1);

        len -= (PMM::PAGE_SIZE - off);
        if (len <= 0) cmd.Prp2 = 0;
        else
        {
            u64 addr = u64(info) + (PMM::PAGE_SIZE - off);
            cmd.Prp2 = addr;
        }

#define NVME_CAPMPSMIN(cap) (((cap) >> 48) & 0xf)
        u16 status = m_AdminQueue->AwaitSubmit(&cmd);
        if (status) return -1;

        usize shift     = 12 + NVME_CAPMPSMIN(m_Register->Capabilities);
        m_MaxTransShift = 20;
        if (info->MaxDataTransferSize)
            m_MaxTransShift = shift + info->MaxDataTransferSize;

        return 0;
    }
    bool Controller::DetectNameSpaces(u32 namespaceCount)
    {
        u32* namespaceIDs = reinterpret_cast<u32*>(
            new u8[Math::AlignUp(namespaceCount * 4, PMM::PAGE_SIZE)]);
        Submission getNamespace   = {};
        getNamespace.OpCode       = OpCode::ADMIN_IDENTIFY;
        getNamespace.Identify.Cns = 2;
        getNamespace.Prp1         = Pointer(namespaceIDs).FromHigherHalf<u64>();
        AssertFmt(!m_AdminQueue->AwaitSubmit(&getNamespace),
                  "NVMe: Failed to acquire namespaces for controller {}",
                  m_Index);

        SetQueueCount(4);
        for (usize i = 0; i < namespaceCount; i++)
        {
            u32 namespaceID = namespaceIDs[i];
            if (namespaceID && namespaceID < namespaceCount)
            {
                LogTrace("NVMe: Found namespace #{:#x}", namespaceID);
                AddNameSpace(namespaceID);
            }
        }

        delete namespaceIDs;
        return true;
    }

    isize Controller::SetQueueCount(i32 count)
    {
        Submission cmd     = {};
        cmd.OpCode         = OpCode::ADMIN_SETFT;
        cmd.Prp1           = 0;
        cmd.Features.Fid   = 0x07;
        cmd.Features.Dword = (count - 1) | ((count - 1) << 14);
        u16 status         = m_AdminQueue->AwaitSubmit(&cmd);
        if (status) return -1;

        return 0;
    }
    bool Controller::AddNameSpace(u32 namespaceID)
    {
        NameSpace* nameSpace = new NameSpace(namespaceID, this);
        if (!nameSpace) return false;
        if (!nameSpace->Initialize())
        {
            delete nameSpace;
            return false;
        }

        m_NameSpaces[namespaceID] = nameSpace;
        return true;
    }
}; // namespace NVMe
