/*
 * Created by v1tr10l7 on 17.05.2025.
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

export module Cryptix:Drivers.Virtio.VirtioDevice;

export import :Drivers.Core.Device;
export import :Drivers.PCI.Device;

export namespace Virtio
{
    enum class ConfigurationType : u8
    {
        eCommon               = 1,
        eNotify               = 2,
        eISR                  = 3,
        eDevice               = 4,
        ePCICapabilitiesAccess = 5,
    };
    struct [[gnu::packed]] Configuration
    {
        ConfigurationType Type;
        u8                ResourceID;
        u32               Offset;
        u32               Length;
    };

    class Device : PCI::Device
    {
      public:
        Device(const PCI::DeviceAddress& address);

        void Initialize();
        void Reset();
        void Configure();

        void Start();
        void Stop();

      private:
        Configuration             m_CommonCfg;
        Configuration             m_NotifyCfg;
        Configuration             m_IsrCfg;
        UnorderedMap<u8, Pointer> m_RegisterBases;
        Vector<Configuration>     m_Configurations;

        void                      ParseCapabilities();
    };
}; // namespace Virtio
