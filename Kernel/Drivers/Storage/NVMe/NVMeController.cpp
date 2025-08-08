/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/Core/DeviceManager.hpp>
#include <Drivers/Storage/NVMe/NVMeController.hpp>
#include <Drivers/Storage/NVMe/NVMeQueue.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace NVMe
{
    Atomic<usize> Controller::s_ControllerCount = 0;

    Controller::Controller(const PCI::DeviceAddress& address)
        : PCI::Device(address)
        , CharacterDevice(MakeDevice(241, s_ControllerCount.Load()))
        , m_Index(s_ControllerCount++)
    {
        m_Name += StringUtils::ToString(m_Index);

        LogTrace("NVMe{}: Initializing...", m_Index);
        if (!Initialize())
        {
            LogError("NVMe{}: Failed to initialize", m_Index);
            return;
        }

        DeviceManager::RegisterCharDevice(this);
    }

    bool Controller::Initialize()
    {
        EnableMemorySpace();
        EnableBusMastering();
        PCI::Bar bar = GetBar(0);
        LogInfo(
            "NVMe{}: bar0 = {{ Base: {:#x}, Address: {:#x}, Size: {:#x}, "
            "IsMMIO: {} }}",
            m_Index, bar.Base.Raw<u64>(), bar.Address.Raw<u64>(), bar.Size,
            bar.IsMMIO);

        if (!bar)
        {
            LogError("NVMe: Failed to acquire BAR0!");
            return false;
        }

        m_CrAddress = bar.Map(0);

        m_Register  = m_CrAddress.As<volatile ControllerRegister>();

        AssertMsg(bar.IsMMIO, "PCI bar is not memory mapped!");
        Assert((ReadAt(0x10, 4) & 0b111) == 0b100);

        auto nullOrError = Disable();
        if (!nullOrError) return nullOrError.error();

        m_DoorbellStride = m_Register->Capabilities.DoorbellStride;
        m_QueueSlots     = m_Register->Capabilities.MaxQueueSize;
        u32 queueId      = 0;

        m_AdminQueue
            = new Queue(m_CrAddress, queueId, m_DoorbellStride, m_QueueSlots);
        // TODO(v1tr10l7): Set Msi-X irq

        Pointer asq = m_AdminQueue->GetSubmit();
        Pointer acq = m_AdminQueue->GetComplete();

        m_Register->AdminQueueAttributes.SubmitQueueSize   = m_QueueSlots - 1;
        m_Register->AdminQueueAttributes.CompleteQueueSize = m_QueueSlots - 1;

        m_Register->AdminSubmissionQueue                 = asq.FromHigherHalf();
        m_Register->AdminCompletionQueue                 = acq.FromHigherHalf();

        m_Register->Configuration.IoSubmitQueueEntrySize = 6;
        m_Register->Configuration.IoCompleteQueueEntrySize = 4;
        m_Register->Configuration.Enable                   = true;

        if (!Enable())
        {
            LogError("NVMe: Failed to enable the controller");
            return false;
        }

        ControllerInfo* info = new ControllerInfo;
        AssertFmt(!this->Identify(info),
                  "NVMe{}: Failed to identify the controller", m_Index);

        usize namespaceCount = info->NamespaceCount;
        delete info;

        LogInfo("NVMe: Controller #{} initialized successfully", m_Index);

        return DetectNameSpaces(namespaceCount);
    }
    void          Controller::Shutdown() {}

    void          Controller::Reset() {}

    ErrorOr<void> Controller::Disable()
    {
        m_Register->Configuration.Enable = false;

        return WaitReady(false);
    }
    ErrorOr<void> Controller::Enable()
    {
        m_Register->Configuration.Enable = true;

        return WaitReady(true);
    }
    ErrorOr<void> Controller::WaitReady(bool waitOn)
    {
        for (;;)
        {
            volatile bool status = m_Register->Status & Bit(0);
            if (status == waitOn) break;

            Arch::Pause();
        }
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
        i64        len   = sizeof(ControllerInfo);
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

        u16 status = m_AdminQueue->AwaitSubmit(&cmd);
        if (status) return -1;

        usize shift     = 12 + m_Register->Capabilities.MinMemoryPageSize;
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
        cmd.Features.Dword = (count - 1) | ((count - 1) << 16);
        u16 status         = m_AdminQueue->AwaitSubmit(&cmd);
        if (status) return -1;

        return 0;
    }
    bool Controller::AddNameSpace(u32 namespaceID)
    {
        auto nameFormatted = fmt::format("{}n{}", Name(), namespaceID);
        auto name = StringView(nameFormatted.data(), nameFormatted.size());

        NameSpace* nameSpace = new NameSpace(name, namespaceID, this);
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
