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
#include <uacpi/resources.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

export module Cryptix:Drivers.PCI.HostController;

export import :Drivers.PCI.Bus;
export import :Drivers.PCI.Device;
export import :Drivers.PCI.PCI;

export namespace PCI
{
    using Enumerator = Delegate<bool(const DeviceAddress&)>;

    class HostController
    {
      public:
        HostController(const Domain& domain, Pointer address)
            : m_Domain(domain)
            , m_Address(address)
        {
        }

        void Initialize();
        void InitializeIrqRoutes();

        struct IrqRoute
        {
            u64  Gsi;
            i32  Device;
            i32  Function;
            u8   Pin;
            bool EdgeTriggered;
            bool ActiveHigh;
        };

        inline const Domain&   GetDomain() const { return m_Domain; }
        inline const auto&     GetIrqRoutes() const { return s_IrqRoutes; }
        inline const IrqRoute* FindIrqRoute(i32 device, u8 pin) const
        {
            for (const auto& entry : s_IrqRoutes)
                if (entry.Device == device && entry.Pin == pin) return &entry;
            return nullptr;
        }

        Device* FindDeviceByID(const DeviceID& id);
        bool    EnumerateDevices(Enumerator enumerator);

        u32     Read(const DeviceAddress& dev, u32 offset, i32 accessSize)
        {
            Assert(m_AccessMechanism);
            return m_AccessMechanism->Read(dev, offset, accessSize);
        }
        void Write(const DeviceAddress& dev, u32 offset, u32 value,
                   i32 accessSize)
        {
            Assert(m_AccessMechanism);
            return m_AccessMechanism->Write(dev, offset, value, accessSize);
        }

        template <UnsignedIntegral T>
            requires(sizeof(T) <= 4)
        T Read(const DeviceAddress& address, u32 offset)
        {
            return Read(address, offset, sizeof(T));
        }
        template <UnsignedIntegral T>
            requires(sizeof(T) <= 4)
        void Write(const DeviceAddress& address, u32 offset, T value)
        {
            return Write(address, offset, value, sizeof(T));
        }

      private:
        Domain                         m_Domain{};
        AccessMechanism*               m_AccessMechanism = nullptr;
        Vector<Bus*>                   m_RootBuses{};

        Pointer                        m_Address  = nullptr;
        uacpi_namespace_node*          m_RootNode = nullptr;

        static Vector<IrqRoute>        s_IrqRoutes;

        UnorderedMap<Pointer, Pointer> m_MappedBuses;

        uintptr_t GetAddress(const DeviceAddress& address, u64 offset);

        bool      EnumerateRootBus(Enumerator enumerator);
    };
}; // namespace PCI
