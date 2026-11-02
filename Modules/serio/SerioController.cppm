/*
 * Created by v1tr10l7 on 10.08.2025.
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

export module Cryptix.Modules.Serio;

import Cryptix;

export class SerioController : public CharacterDevice
{
  public:
    enum class DevicePort
    {
        eNone  = 0x00,
        ePort1 = 0x01,
        ePort2 = 0x02,
    };

    enum class DeviceCommand
    {
        eIdentify        = 0xf2,
        eEnableScanning  = 0xf4,
        eDisableScanning = 0xf5,
        eReset           = 0xff,
    };

    static ErrorOr<void>          Register(::Ref<SerioController> ctrl);
    static ::Ref<SerioController> Instance();

    virtual ~SerioController()                           = default;

    virtual bool          IsOutputEmpty()                = 0;

    virtual ErrorOr<void> EnableDevice(DevicePort port)  = 0;
    virtual ErrorOr<void> DisableDevice(DevicePort port) = 0;

    virtual ErrorOr<void> EnablePort1Translation()       = 0;
    virtual ErrorOr<void> DisablePort1Translation()      = 0;

    virtual ErrorOr<void> ResetDevice(DevicePort port)   = 0;

    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command)
        = 0;
    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command, u8 data)
        = 0;

  protected:
    static SerioController* s_Instance;

    SerioController(StringView name, DeviceMajor major, DeviceMinor minor)
        : CharacterDevice(name, major, minor)
    {
    }
};
export using SerioDevicePort = SerioController::DevicePort;

export class SerioDevice : public InputDevice
{
  public:
    inline SerioDevice(StringView name, SerioController* serio)
        : InputDevice(name, InputDevice::AllocateMinor())
    {
    }
    virtual void OnByteReceived(u8 byte) = 0;
};
