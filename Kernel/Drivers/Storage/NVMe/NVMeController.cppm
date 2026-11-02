/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Memory/Scope.hpp>

export module Cryptix:Drivers.Storage.NVMe.NVMeController;

export import :Drivers.Core.CharacterDevice;
export import :Drivers.PCI.Device;
export import :Drivers.Storage.NVMe.NVMeNameSpace;
export import :Drivers.Storage.NVMe.NVMeQueue;

export namespace NVMe
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

    struct [[gnu::packed]] ControllerInfo
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
    };

    constexpr inline Configuration operator|(Configuration lhs,
                                             Configuration rhs)
    {
        auto result = ToUnderlying(lhs) | ToUnderlying(rhs);

        return static_cast<Configuration>(result);
    }
    constexpr inline Configuration operator~(Configuration conf)
    {
        auto result = ~ToUnderlying(conf);

        return static_cast<Configuration>(result);
    }
    constexpr inline Configuration& operator&=(Configuration& lhs,
                                               Configuration  rhs)
    {
        auto result = ToUnderlying(lhs);
        result &= ToUnderlying(rhs);

        return lhs = static_cast<Configuration>(result);
    }

    constexpr inline bool operator&(Status lhs, Status rhs)
    {
        auto result = ToUnderlying(lhs) & ToUnderlying(rhs);

        return result;
    }

    struct [[gnu::packed]] ControllerCapabilities
    {
        u64  MaxQueueSize                  : 16;
        bool ContiguousQueuesRequired      : 1;
        u64  ArbitrationMechanismSupported : 2;
        u64  Reserved                      : 5;
        u64  Timeout                       : 8;
        u64  DoorbellStride                : 4;
        bool SubsystemReset                : 1;
        u8   CommandSets                   : 8;
        u64  BootPartition                 : 1;
        u64  PowerScope                    : 2;
        u64  MinMemoryPageSize             : 4;
        u64  MaxMemoryPageSize             : 4;
        u64  PersistentMemoryRegion        : 1;
        u64  MemoryBuffer                  : 1;
        bool SubsystemShutdown             : 1;
        u64  ReadyModes                    : 2;
        u64  Reserved1                     : 3;
    };
    struct [[gnu::packed]] ControllerConfiguration
    {
        bool Enable                   : 1;
        u8   Reserved0                : 3;
        u8   CommandSet               : 3;
        u8   MemoryPageSize           : 4;
        u8   ArbitrationMechanism     : 3;
        u8   ShutdownNotification     : 2;
        u8   IoSubmitQueueEntrySize   : 4;
        u8   IoCompleteQueueEntrySize : 4;
        u8   Crime                    : 1;
        u8   Reserved1                : 7;
    };
    struct [[gnu::packed]] AdminQueuettributes
    {
        u32 SubmitQueueSize   : 12;
        u32 Reserved0         : 4;
        u32 CompleteQueueSize : 12;
        u32 Reserved1         : 4;
    };
    struct [[gnu::packed]] ControllerRegister
    {
        ControllerCapabilities  Capabilities;
        u32                     Version;
        u32                     InterruptMaskSet;
        u32                     InterruptMaskClear;
        ControllerConfiguration Configuration;
        u32                     Reserved;
        u32                     Status;
        u32                     Reserved2;
        AdminQueuettributes     AdminQueueAttributes;
        u64                     AdminSubmissionQueue;
        u64                     AdminCompletionQueue;
    };

    class Controller : public PCI::Device, public CharacterDevice
    {
      public:
        explicit Controller(const PCI::DeviceAddress& address);

        bool          Initialize();
        void          Shutdown();

        void          Reset();

        ErrorOr<void> Disable();
        ErrorOr<void> Enable();
        ErrorOr<void> WaitReady(bool waitOn);

        inline Queue* GetAdminQueue() const { return m_AdminQueue; }
        inline usize  GetMaxTransShift() const { return m_MaxTransShift; }

        bool          CreateIoQueues(NameSpace& ns, Queue*& queues, u32 id);

        virtual StringView     Name() const noexcept override { return m_Name; }

        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override
        {
            return 0;
        }
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override
        {
            return 0;
        }

        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override
        {
            return 0;
        }
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override
        {
            return 0;
        }

        virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
        {
            return Error(ENOSYS);
        }

      private:
        String m_Name                                    = "nvme"_s;
        usize  m_Index                                   = 0;
        ControllerRegister volatile* volatile m_Register = nullptr;
        Pointer                       m_CrAddress;

        u64                           m_QueueSlots     = 0;
        u64                           m_DoorbellStride = 0;

        Spinlock                      m_Lock;
        class Queue*                  m_AdminQueue    = nullptr;
        usize                         m_MaxTransShift = 0;
        UnorderedMap<u32, NameSpace*> m_NameSpaces;

        static Atomic<usize>          s_ControllerCount;

        i32                           Identify(ControllerInfo* info);
        bool                          DetectNameSpaces(u32 namespaceCount);

        isize                         SetQueueCount(i32 count);
        bool                          AddNameSpace(u32 id);
    };
}; // namespace NVMe
