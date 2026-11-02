/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
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
#include <Prism/Core/TypeTraits.hpp>

export module Cryptix:Drivers.USB.UHCI.UHCiController;

#if defined(CTOS_TARGET_X86_64)
export import :Arch.x86_64.IO;
#endif
export import :Drivers.Core.Device;
export import :Drivers.PCI.Device;
export import :Drivers.USB.HostController;
export import :Drivers.USB.UHCI.Definitions;
export import :Drivers.USB.UHCI.UHCiDescriptorPool;
export import :Memory.IORegion;

#ifdef CTOS_TARGET_X86_64
#endif

export namespace USB::UHCI
{
    class Controller : public HostController, public PCI::Device, public Device
    {
      public:
        explicit Controller(const PCI::DeviceAddress& address);

        virtual ErrorOr<void> Initialize() override;

        virtual ErrorOr<void> Start() override;
        virtual ErrorOr<void> Stop() override;

        virtual ErrorOr<void> Reset() override;

        virtual StringView Name() const noexcept override { return "uhci0"_sv; }

        virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                    isize offset = -1) override;
        virtual ErrorOr<isize> Read(void* dest, off_t offset,
                                    usize bytes) override;
        virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                     usize bytes) override;
        virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                     isize offset = -1) override;

        virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override;

      private:
        using Bar = PCI::Bar;
        Bar                    m_Bar;

        Pointer                m_FrameListPhys = 0;
        struct FrameListEntry* m_FrameList;
        IoRegion               m_IoRegisters;
        // DescriptorPool<QueueHead>          m_QueueHeadPool;

        CTOS_UNUSED Vector<QueueHead*> m_FreeQhPool;
        CTOS_UNUSED Vector<TransferDescriptor*> m_FreeTdPool;
        CTOS_UNUSED Vector<TransferDescriptor*> m_IsoTdList;

        CTOS_UNUSED QueueHead* m_InterruptTransferQueue = nullptr;
        CTOS_UNUSED QueueHead* m_LowSpeedControlQh      = nullptr;
        CTOS_UNUSED QueueHead* m_FullSpeedControlQh     = nullptr;
        CTOS_UNUSED QueueHead* m_BulkQh                 = nullptr;
        CTOS_UNUSED QueueHead* m_DummyQh                = nullptr;

        CTOS_UNUSED Pointer    m_QhPool                 = nullptr;
        CTOS_UNUSED Pointer    m_TdPool                 = nullptr;

        QueueHead*             AllocateQueueHead();
        TransferDescriptor*    AllocateTransferDescriptor() const;

        //
        // QueueHead*                         m_ScheduleBeginAnchor;
        // QueueHead*                         m_InterruptQhAnchor;
        // QueueHead*                         m_LsControlQhAnchor;
        // QueueHead*                         m_FsControlQhAnchor;
        // QueueHead*                         m_BulkQhAnchor;
        //
        // DescriptorPool<TransferDescriptor> m_TransferPool;
        // DescriptorPool<TransferDescriptor> m_IsochronousTransferPool;

        enum class Register : u16
        {
            eCommand              = 0x00,
            eStatus               = 0x02,
            eInterruptEnable      = 0x04,
            eFrameNumber          = 0x06,
            eFrameListBaseAddress = 0x08,
            eStartOfFrameModify   = 0x0c,
            ePort1StatusControl   = 0x10,
            ePort2StatusControl   = 0x12,
        };
        enum class Command : u16
        {
            eRun                 = Bit(0),
            eHostControllerReset = Bit(1),
            eGlobalReset         = Bit(2),
            eGlobalSuspend       = Bit(3),
            eGlobalResume        = Bit(4),
            eSoftwareDebug       = Bit(5),
            eConfigureFlag       = Bit(6),
            e64BitPacketSize     = Bit(7),
        };
        friend inline constexpr Command operator~(const Command& op)
        {
            return static_cast<Controller::Command>(~ToUnderlying(op));
        }
        friend inline constexpr Command operator|(Command lhs, Command rhs)
        {
            return static_cast<Command>(ToUnderlying(lhs) | ToUnderlying(rhs));
        }
        friend inline constexpr u64 operator|(u64 lhs, Command rhs)
        {
            return lhs | ToUnderlying(rhs);
        }
        friend inline constexpr u64 operator&(u64 lhs, Command rhs)
        {
            return lhs & ToUnderlying(rhs);
        }

        enum class Status : u16
        {
            eInterrupt      = Bit(0),
            eErrorInterrupt = Bit(1),
            eResumeDetected = Bit(2),
            eSystemError    = Bit(3),
            eProcessError   = Bit(4),
            eHalted         = Bit(5),
        };
        friend inline constexpr u64 operator&(u64 lhs, Status rhs)
        {
            return lhs & ToUnderlying(rhs);
        }
        enum class InterruptEnable
        {
            eTimeOutCRC       = Bit(0),
            eResume           = Bit(1),
            eCompleteTransfer = Bit(2),
            eShortPacket      = Bit(3),
        };
        friend inline constexpr InterruptEnable operator|(InterruptEnable lhs,
                                                          InterruptEnable rhs)
        {
            return static_cast<InterruptEnable>(ToUnderlying(lhs)
                                                | ToUnderlying(rhs));
        }

        void          HandleInterrupt();

        constexpr u64 Read(Register reg) const
        {
            if (reg == Register::eStartOfFrameModify)
                return m_IoRegisters.Read<u8>(ToUnderlying(reg));
            else if (reg == Register::eFrameListBaseAddress)
                return m_IoRegisters.Read<u32>(ToUnderlying(reg));
            else return m_IoRegisters.Read<u16>(ToUnderlying(reg));
            LogError("UHCI: No access mechanism available for pci device");
        }
        constexpr void Write(Register reg, u64 value) const
        {
            if (reg == Register::eStartOfFrameModify)
                m_IoRegisters.Write<u8>(ToUnderlying(reg), value & 0xff);
            else if (reg == Register::eFrameListBaseAddress)
                m_IoRegisters.Write<u32>(ToUnderlying(reg), value & 0xffffffff);
            else m_IoRegisters.Write<u16>(ToUnderlying(reg), value & 0xffff);
        }

        template <typename T>
        constexpr T Read(Register reg) const
            requires(IsEnumV<T>)
        {
            return static_cast<T>(Read(reg));
        }
        template <typename T>
        constexpr void Write(Register reg, T value) const
            requires(IsEnumV<T>)
        {
            return Write(reg, ToUnderlying(value));
        }
    };
}; // namespace USB::UHCI
