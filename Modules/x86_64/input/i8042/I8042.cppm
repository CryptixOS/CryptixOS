/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#include <Prism/Core/Concepts.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/String/String.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <compare>
#include <API/UnixTypes.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
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
#include <Prism/String/StringUtils.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>

export module Cryptix.Modules.I8042;

import Cryptix;
import Cryptix.Modules.Serio;
export import :PNP_IDs;

export enum class Ps2DeviceType
{
    eUndefined = 0,
    eATKeyboard,
    eStandardMouse,
    eScrollWheelMouse,
    e5ButtonMouse,
    eMf2Keyboard,
    eThinkPadKeyboard,
    eNcdKeyboard,
    eHostConnected122KeysKeyboard,
    eStandardKeyboard,
    eJapaneseGKeyboard,
    eJapanesePKeyboard,
    eJapaneseAKeyboard,
    eNcdSunKeyboard
};

export class I8042 : public ACPI::Device, public SerioController
{
  public:
    I8042(ACPI::DeviceHandle* handle, DeviceMajor major, DeviceMinor minor)
        : ACPI::Device(handle)
        , SerioController("i8042", 200, 200)
    {
    }

    enum class Port : u16
    {
        eBuffer  = 0x60,
        eCommand = 0x64,
        eStatus  = 0x64,
    };
    enum class Status : u8
    {
        // Data sent from ps/2 controller
        eOutBufferFull = Bit(0),
        // Data to be read by ps/2 controller
        eInBufferFull  = Bit(1),
        eSystem        = Bit(2),
        // if 0, then data written to input buffer is data for ps/2 device,
        // otherwise it's a command for ps/2 controller
        eCommand       = Bit(3),
        eTimeOutError  = Bit(6),
        eParityError   = Bit(7),
    };
    enum class Command : u8
    {
        eReadConfigurationByte     = 0x20,
        eWriteConfigurationByte    = 0x60,
        eDisablePort2              = 0xa7,
        eEnablePort2               = 0xa8,
        eTestPort2                 = 0xa9,
        eTestController            = 0xaa,
        eTestPort1                 = 0xab,
        eDiagnosticDump            = 0xac,
        eDisablePort1              = 0xad,
        eEnablePort1               = 0xae,
        eReadControllerInput       = 0xc0,
        // Copy bits (0-3) of input port to status bits(4-7)
        eCopyInputLow2StatusHigh   = 0xc1,
        // Copy bits (4-7) of input port to status bits(4-7)
        eCopyInputHigh2StatusHigh  = 0xc1,
        eReadControllerOutput      = 0xd0,
        eWriteNextControllerOutput = 0xd1,
        eWriteNextOutPort1         = 0xd2,
        eWriteNextOutPort2         = 0xd3,
        eWriteToPort2              = 0xd4,
        eResetCPU                  = 0xfe,
    };
    enum class Configuration : u8
    {
        eEnablePort1Irq         = Bit(0),
        eEnablePort2Irq         = Bit(1),
        eSystemPassedPost       = Bit(2),
        eDisablePort1Clock      = Bit(4),
        eDisablePort2Clock      = Bit(5),
        eEnablePort1Translation = Bit(6),
    };

    enum class Output
    {
        eSystemReset = Bit(0),
        eA20Gate     = Bit(1),
        ePort2Clock  = Bit(2),
        ePort2Data   = Bit(3),
        ePort1Clock  = Bit(6),
        ePort1Data   = Bit(7),
    };

    enum Response
    {
        ePortTestSuccess        = 0x00,
        ePortClockLineStuckLow  = 0x01,
        ePortClockLineStuckHigh = 0x02,
        ePortDataLineStuckLow   = 0x03,
        ePortDataLineStuckHigh  = 0x04,

        eSelfTestSuccess        = 0x55,
        eDeviceTestPassed       = 0xaa,
        eAcknowledge            = 0xfa,
        eResend                 = 0xfe,
    };

    inline static I8042* Instance() { return s_Controller; }

    ErrorOr<void>        Initialize();

    ErrorOr<void> RegisterPort(SerioDevicePort port, ::Ref<SerioDevice> device);

    virtual bool  IsOutputEmpty();
    bool          IsInputEmpty();

    u8            ReadBlocking();
    void          WriteBlocking(Port port, u8 data);

    ErrorOr<u8>   TryRead();
    ErrorOr<void> TryWrite(Port port, u8 data);

    ErrorOr<void> FlushReadBuffer();
    ErrorOr<void> SendCommand(Command command);
    /*{
        WriteBlocking(Port::eCommand, ToUnderlying(command));
    }*/

    ErrorOr<u8>   ReadDevicePort(DevicePort port);
    ErrorOr<void> WriteDevicePort(DevicePort port, byte data);

    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command)
    {
        return WriteDevicePort(port, ToUnderlying(command));
    }
    virtual ErrorOr<void> SendDeviceCommand(DevicePort    port,
                                            DeviceCommand command, u8 data)
    {
        auto successOr = SendDeviceCommand(port, command);
        if (!successOr) return successOr;
        successOr = WriteDevicePort(port, data);
        if (!successOr) return successOr;

        return {};
    }

    virtual ErrorOr<void> EnableDevice(DevicePort port);
    virtual ErrorOr<void> DisableDevice(DevicePort port);

    virtual ErrorOr<void> EnablePort1Translation();
    virtual ErrorOr<void> DisablePort1Translation();
    virtual ErrorOr<void> ResetDevice(DevicePort port);

    static ErrorOr<void>  Probe(ACPI::DeviceHandle* handle, StringView);
    static void           Remove(ACPI::DeviceHandle*);

  private:
    static constexpr isize     READ_WRITE_TIMEOUT = 10'000;
    static I8042*              s_Controller;

    Spinlock                   m_Lock;
    ::Ref<InterruptDispatcher> m_IrqDispatcher  = nullptr;

    bool                       m_Port1Available = false;
    bool                       m_Port2Available = false;

    static bool                QuerySupport();
    static IrqResult           HandleInterrupt(::Device*         device,
                                               ExecutionContext* context);

    ErrorOr<void>              DisableDevices();

    bool                       PerformSelfTest();
    bool                       IsDualChannel();

    bool                       TestInterfaces();
    bool                       TestSingleInterface(DevicePort port);

    void                       EnumerateDevices();
    ErrorOr<Ps2DeviceType>     ScanPortForDevices(DevicePort port);

    ErrorOr<void>              WaitForIncomingData();
    ErrorOr<void>              WaitForWriteReady();

    Status                     ReadStatus();
    u8                         ReadPort(Port port);
    void                       WritePort(Port port, u8 data);
};

export using I8042Port          = I8042::Port;
export using I8042Command       = I8042::Command;
export using I8042Configuration = I8042::Configuration;
export using I8042Response      = I8042::Response;
export using PS2Port            = I8042::DevicePort;

export inline u8 operator~(I8042Configuration lhs) { return ~ToUnderlying(lhs); }
export inline u8 operator&(u8 lhs, const I8042Configuration rhs)
{
    return lhs & ToUnderlying(rhs);
}
export inline I8042Configuration operator|(I8042Configuration       lhs,
                                    const I8042Configuration rhs)
{
    u8 config = ToUnderlying(lhs) | ToUnderlying(rhs);

    return static_cast<I8042Configuration>(config);
}
export inline u8& operator&=(u8& lhs, const I8042Configuration rhs)
{
    lhs &= ToUnderlying(rhs);

    return lhs;
}
export inline u8& operator|=(u8& lhs, const I8042Configuration rhs)
{
    lhs |= ToUnderlying(rhs);

    return lhs;
}
