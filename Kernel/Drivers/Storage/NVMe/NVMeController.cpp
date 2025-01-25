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

#include <Utility/Math.hpp>
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
        VFS::MkNod(VFS::GetRootNode(), std::format("/dev/{}", GetName()), 0666,
                   GetID());

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

        usize   bar1      = PCI::Device::Read<u32>(PCI::RegisterOffset::eBar1);

        Pointer crAddress = (bar.Address.Raw<u64>() & -8ull) | bar1 << 32;
        m_Register        = crAddress.As<volatile ControllerRegister>();

        AssertMsg(bar.IsMMIO, "PCI bar is not memory mapped!");
        Assert((ReadAt(0x10, 4) & 0b111) == 0b100);

        uintptr_t addr = Math::AlignDown(crAddress.Raw<u64>(), PMM::PAGE_SIZE);
        usize     len  = Math::AlignUp(bar.Size, PMM::PAGE_SIZE);
        Assert(VMM::GetKernelPageMap()->MapRange(addr, addr, len,
                                                 PageAttributes::eRW));

        LogTrace("NVMe: Successfully mapped bar0");

        u32 cc = m_Register->Configuration;
        if (cc & Bit(0))
        {
            cc &= ~Bit(0);
            m_Register->Configuration = cc;
        }

        while (m_Register->Status & Bit(0)) Arch::Pause();

        usize doorbellStride = (m_Register->Capabilities >> 32) & 0xf;
        usize queueSlots     = std::min(m_Register->Capabilities & 0xffff,
                                        (m_Register->Capabilities >> 16) & 0xffff);
        u32   queueId        = 0;

        LogTrace("NVMe: Creating admin queue...");
        volatile u32* submitDoorbell = reinterpret_cast<volatile u32*>(
            crAddress.Offset<u64>(PMM::PAGE_SIZE)
            + (2 * queueId * (4 << doorbellStride)));

        volatile u32* completeDoorbell = reinterpret_cast<volatile uint32_t*>(
            crAddress.Offset<u64>(PMM::PAGE_SIZE)
            + ((2 * id + 1) * (4 << doorbellStride)));

        u64 doorbell = u64(completeDoorbell);
        VMM::GetKernelPageMap()->MapRange(
            doorbell, doorbell, PMM::PAGE_SIZE * 4, PageAttributes::eRW);

        m_AdminQueue = new Queue(submitDoorbell, completeDoorbell, queueId, 0,
                                 queueSlots);

        u32 aqa      = queueSlots - 1;
        aqa |= aqa << 16;
        aqa |= aqa << 16;
        m_Register->AdminQueueAttributes = aqa;
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
        // TODO(v1tr10l7): Enumerate namespaces
        return true;
    }
    void Controller::Shutdown() {}

    void Controller::Reset() {}

    i32  Controller::Identify(ControllerInfo* info)
    {
        LogDebug("sizeof(Submission) = {}", sizeof(Submission));
        LogDebug("sizeof(Completion) = {}", sizeof(Completion));

        u64        len      = sizeof(ControllerInfo);
        Submission cmd      = {};
        cmd.Identify.opcode = 0x06;
        cmd.Identify.nsid   = 0;
        cmd.Identify.cns    = 1;
        cmd.Identify.prp1   = FromHigherHalfAddress<uintptr_t>(info);
        i32 off             = u64(info) & (PMM::PAGE_SIZE - 1);

        len -= (PMM::PAGE_SIZE - off);
        if (len <= 0) cmd.Identify.prp2 = 0;
        else
        {
            u64 addr          = u64(info) + (PMM::PAGE_SIZE - off);
            cmd.Identify.prp2 = addr;
        }

#define NVME_CAPMPSMIN(cap) (((cap) >> 48) & 0xf)
        u16 status = m_AdminQueue->AwaitSubmit(&cmd);
        if (status) return -1;

        // usize shift         = 12 + NVME_CAPMPSMIN(m_Register->Capabilities);
        // usize maxTransShift = 20;
        // if (info.Mdts) maxTransShift = shift + info.Mdts;

        return 0;
    }
}; // namespace NVMe
