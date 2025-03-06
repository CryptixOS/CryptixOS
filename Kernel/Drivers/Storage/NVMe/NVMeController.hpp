/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/PCI/Device.hpp>
#include <Drivers/Storage/NVMe/NVMeNameSpace.hpp>
#include <Drivers/Storage/NVMe/NVMeQueue.hpp>

#include <atomic>
#include <utility>

namespace NVMe
{
    enum class Configuration : u32
    {
        eEnable = Bit(0),
        eIoSqEs = 6 << 16,
        eIoCqEs = 4 << 20,
    };
    enum class Status : u32
    {
        eReady = Bit(0),
        eCfs   = Bit(1),
    };

    struct ControllerInfo
    {
        u16 VendorID;
        u16 SubsystemVendorID;
        u8  SerialNumber[20];
        u8  ModelNumber[40];
        u8  FirmwareRevision[8];
        u8  RecommendedArbitrationBurst;
        u8  IeeeOuiIdentifier[3];
        u8  MultiPathIoCapabilities;
        u8  MaxDataTransferSize;
        u16 ControllerID;
        u32 Version;
        u32 Unused1[43];
        u16 OptionalAdminCmdSupport;
        u8  AbortCmdLimit;
        u8  AsyncEventRequestLimit;
        u8  FirmwareUpdates;
        u8  LogPageAttributes;
        u8  ErrorLogPageEntries;
        u8  PowerStatesSupportCount;
        u8  AdminVendorSpecificCmdConf;
        u8  AutonomousPowerStateAttribs;
        u16 WarningCompositeTempTreeshold;
        u16 CriticalCompositeTempTreeshold;
        u16 Unused2[121];
        u8  SubmissionQueueEntrySize;
        u8  CompletionQueueEntrySize;
        u16 Unused3;
        u32 NamespaceCount;
        u16 OptionalNvmCmdSupport;
        u16 FusedOperationSupport;
        u8  FormatNvmAttribs;
        u8  VolatileWriteCache;
        u16 AtomicWriteUnitNormal;
        u16 AtomicWriteUnitPowerFail;
        u8  IoCmdSetVendorSpecificConf;
        u8  Unused4;
        u16 AtomicCompareWriteUnit;
        u16 Unused5;
        u32 SglSupport;
        u32 Unused6[1401];
        u8  VendorSpecific[1024];
    } __attribute__((packed));

    inline Configuration operator|(Configuration lhs, Configuration rhs)
    {
        auto result = std::to_underlying(lhs) | std::to_underlying(rhs);

        return static_cast<Configuration>(result);
    }
    inline Configuration operator~(Configuration conf)
    {
        auto result = ~std::to_underlying(conf);

        return static_cast<Configuration>(result);
    }
    inline Configuration& operator&=(Configuration& lhs, Configuration rhs)
    {
        auto result = std::to_underlying(lhs);
        result &= std::to_underlying(rhs);

        return lhs = static_cast<Configuration>(result);
    }

    inline bool operator&(Status lhs, Status rhs)
    {
        auto result = std::to_underlying(lhs) & std::to_underlying(rhs);

        return result;
    }

    struct ControllerRegister
    {
        u64 Capabilities;
        u32 Version;
        u32 InterruptMaskSet;
        u32 InterruptMaskClear;
        u32 Configuration;
        u32 Reserved;
        u32 Status;
        u32 Reserved2;
        u32 AdminQueueAttributes;
        u64 AdminSubmissionQueue;
        u64 AdminCompletionQueue;
    } __attribute__((packed));

    class Controller : public PCI::Device
    {
      public:
        Controller(const PCI::DeviceAddress& address);

        bool          Initialize();
        void          Shutdown();

        void          Reset();

        ErrorOr<void> Disable();
        ErrorOr<void> Enable();
        ErrorOr<void> WaitReady();

        inline Queue* GetAdminQueue() const { return m_AdminQueue; }
        inline usize  GetMaxTransShift() const { return m_MaxTransShift; }

        bool          CreateIoQueues(NameSpace& ns, Queue*& queues, u32 id);

        virtual std::string_view GetName() const noexcept override
        {
            return m_Name;
        }

        virtual isize Read(void* dest, off_t offset, usize bytes) override
        {
            return 0;
        }
        virtual isize Write(const void* src, off_t offset, usize bytes) override
        {
            return 0;
        }

        virtual i32 IoCtl(usize request, uintptr_t argp) override { return 0; }

      private:
        std::string m_Name                               = "nvme";
        usize       m_Index                              = 0;
        ControllerRegister volatile* volatile m_Register = nullptr;
        PM::Pointer                         m_CrAddress;

        u64                                 m_QueueSlots     = 0;
        u64                                 m_DoorbellStride = 0;

        Spinlock                            m_Lock;
        class Queue*                        m_AdminQueue    = nullptr;
        usize                               m_MaxTransShift = 0;
        std::unordered_map<u32, NameSpace*> m_NameSpaces;

        static std::atomic<usize>           s_ControllerCount;

        i32                                 Identify(ControllerInfo* info);
        bool  DetectNameSpaces(u32 namespaceCount);

        isize SetQueueCount(i32 count);
        bool  AddNameSpace(u32 id);
    };
}; // namespace NVMe
