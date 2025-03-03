/*
 * Created by v1tr10l7 on 05.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/x86_64/IO.hpp>

#include <Arch/x86_64/Drivers/I8042Controller.hpp>
#include <Arch/x86_64/Drivers/PS2Keyboard.hpp>

#include <magic_enum/magic_enum.hpp>

namespace I8042Controller
{
    namespace
    {
        bool        s_Port1Available = false;
        bool        s_Port2Available = false;

        PS2Keyboard s_Keyboard;

        u8          ReadPort(Port port)
        {
            return IO::In<byte>(std::to_underlying(port));
        }
        void WritePort(Port port, u8 data)
        {
            IO::Out<byte>(std::to_underlying(port), data);
        }

        std::expected<void, errno_t> WaitForAnyOutput()
        {
            for (i32 i = 0; i < 1000; i++)
            {
                if (!IsOutputEmpty()) return {};

                IO::Delay(1000);
            }

            return std::unexpected<errno_t>(EBUSY);
        }
        std::expected<void, errno_t> WaitForEmptyInput()
        {
            for (i32 i = 0; i < 1000; i++)
            {
                if (IsInputEmpty()) return {};

                IO::Delay(1000);
            }

            return std::unexpected<errno_t>(EBUSY);
        }

        void DisableDevices()
        {
            SendCommand(Command::eDisablePort1);
            SendCommand(Command::eDisablePort2);
        }

        bool PerformSelfTest()
        {
            SendCommand(Command::eReadConfigurationByte);
            u8 config = ReadBlocking();

            SendCommand(Command::eTestController);
            if (ReadBlocking() != Response::eSelfTestSuccess) return false;

            // On some hardware the self test might reset the device,
            // so we're writing to the configuration byte again
            SendCommand(Command::eWriteConfigurationByte);
            WriteBlocking(Port::eBuffer, config);

            return true;
        }
        bool IsDualChannel()
        {
            SendCommand(Command::eEnablePort2);

            SendCommand(Command::eReadConfigurationByte);
            u8   config = ReadBlocking();

            bool isDualChannel
                = (config & Configuration::eDisablePort2Clock) == 0;
            if (isDualChannel)
            {
                SendCommand(Command::eDisablePort2);
                config &= ~Configuration::eEnablePort2Irq;
                config &= ~Configuration::eDisablePort2Clock;

                SendCommand(Command::eWriteConfigurationByte);
                WriteBlocking(Port::eBuffer, config);
            }

            return isDualChannel;
        }

        bool TestSingleInterface(DevicePort port)
        {
            Assert(std::to_underlying(port) <= 2);
            LogTrace("I8042Controller: Testing port #{}...",
                     std::to_underlying(port));
            SendCommand(port == DevicePort::ePort1 ? Command::eTestPort1
                                                   : Command::eTestPort2);
            Response status = static_cast<Response>(ReadBlocking());

            if (status == Response::ePortTestSuccess)
            {
                LogTrace("I8042Controller: Port #{} is working correctly",
                         std::to_underlying(port));

                return true;
            }

            LogError("I8042Controller: Port #1 error -> {}",
                     magic_enum::enum_name(status));
            return false;
        }
        bool TestInterfaces()
        {
            LogTrace("I8042Controller: Testing interfaces...");
            s_Port1Available = TestSingleInterface(DevicePort::ePort1);
            s_Port2Available = TestSingleInterface(DevicePort::ePort2);

            return s_Port1Available | s_Port2Available;
        }

        std::expected<void, errno_t> ScanPortForDevices(DevicePort port)
        {
            Assert(port == DevicePort::ePort1 || port == DevicePort::ePort2);
            SendCommand(port == DevicePort::ePort1 ? Command::eEnablePort1
                                                   : Command::eEnablePort2);

            auto status
                = SendDeviceCommand(port, DeviceCommand::eDisableScanning);
            if (!status) return status;

            auto response = TryRead();
            if (!response) return std::unexpected(response.error());
            if (response.value() != Response::eAcknowledge)
                return std::unexpected(ENODEV);

            Flush();
            status = SendDeviceCommand(port, DeviceCommand::eIdentify);
            if (!status) return status;

            response = TryRead();
            if (!response) return std::unexpected(response.error());
            if (response.value() != Response::eAcknowledge)
                return std::unexpected(ENODEV);

            auto byte1 = TryRead();
            auto byte2 = TryRead();

            if (byte1)
                LogInfo("I8042Controller: Device #{} reply[0] = {:#x}",
                        std::to_underlying(port), byte1.value());
            if (byte2)
                LogInfo("I8042Controller: Device #{} reply[1] = {:#x}",
                        std::to_underlying(port), byte2.value());

            // status = ResetDevice(port);
            // if (!status) return status;

            Flush();
            status = SendDeviceCommand(port, DeviceCommand::eEnableScanning);
            if (!status)
            {
                LogError(
                    "I8042Controller: Failed to enable scanning on device #{}",
                    std::to_underlying(port));

                return status;
            }

            response = TryRead();
            if (!response) return std::unexpected(response.error());
            if (response.value() != Response::eAcknowledge)
                return std::unexpected(ENODEV);

            SendCommand(port == DevicePort::ePort1 ? Command::eDisablePort1
                                                   : Command::eDisablePort2);
            Flush();
            return {};
        }
        void EnumerateDevices()
        {
            if (s_Port1Available && ScanPortForDevices(DevicePort::ePort1))
                LogTrace("I8042Controller: Detected device on port #1");
            if (s_Port2Available && ScanPortForDevices(DevicePort::ePort2))
                LogTrace("I8042Controller: Detected device on port #2");

            s_Keyboard = PS2Keyboard(PS2Port::ePort1);
        }
    }; // namespace

    void Initialize()
    {
        LogTrace("I8042Controller: Initializing...");
        DisableDevices();
        Flush();

        SendCommand(Command::eReadConfigurationByte);
        u8 config = ReadBlocking();

        // Disable port1 irq and port1 translation
        config &= ~Configuration::eEnablePort1Irq;
        config &= ~Configuration::eEnablePort1Translation;

        // Disable port2 irq
        config &= ~Configuration::eEnablePort2Irq;

        // Make sure port1 clock signal is enabled
        config &= ~Configuration::eDisablePort1Clock;
        SendCommand(Command::eWriteConfigurationByte);
        WriteBlocking(Port::eBuffer, config);

        if (!PerformSelfTest())
        {
            LogError("I8042Controller: Self Test fail");
            return;
        }

        s_Port2Available = IsDualChannel();
        if (!TestInterfaces())
        {
            LogError("I8042Controller: Both ports failed to pass tests");
            return;
        }

        LogInfo("I8042Controller: Successfully initialized");

        LogTrace("I8042Controller: Enumerating the available PS/2 devices...");
        EnumerateDevices();
    }

    bool IsOutputEmpty()
    {
        return (ReadPort(Port::eStatus)
                & std::to_underlying(Status::eOutBufferFull))
            == 0;
    }
    bool IsInputEmpty()
    {
        return (ReadPort(Port::eStatus)
                & std::to_underlying(Status::eInBufferFull))
            == 0;
    }

    u8 ReadBlocking()
    {
        while (IsOutputEmpty()) Arch::Pause();

        return ReadPort(Port::eBuffer);
    }
    void WriteBlocking(Port port, u8 data)
    {
        while (!IsInputEmpty()) Arch::Pause();

        WritePort(port, data);
    }

    std::expected<u8, errno_t> TryRead()
    {
        auto status = WaitForAnyOutput();
        if (!status) return status.error();

        return ReadPort(Port::eBuffer);
    }
    std::expected<void, errno_t> TryWrite(Port port, u8 data)
    {
        auto status = WaitForEmptyInput();
        if (!status) return status;

        WritePort(port, data);
        return {};
    }

    void Flush()
    {
        while (!IsOutputEmpty()) CtosUnused(ReadPort(Port::eBuffer));
    }

    std::expected<void, errno_t> WriteToDevice(DevicePort port, byte data)
    {

        usize attempts = 0;

        byte  response = 0x00;
        while (attempts++ < 150 || response == Response::eResend)
        {
            auto status = WaitForEmptyInput();
            if (!status) return status;

            if (port == DevicePort::ePort2)
            {
                if (!s_Port2Available) return std::unexpected(ENODEV);
                SendCommand(Command::eWriteToPort2);
            }

            WriteBlocking(Port::eBuffer, data);
            response = ReadBlocking();
        }

        if (attempts >= 150 && response == Response::eResend)
            return std::unexpected(EBUSY);

        WriteBlocking(Port::eBuffer, data);
        return {};
    }

    std::expected<void, errno_t> ResetDevice(DevicePort port)
    {
        Flush();
        auto status = SendDeviceCommand(port, DeviceCommand::eReset);
        if (!status) return status;

        auto byte1 = TryRead();
        auto byte2 = TryRead();

        if (!byte1) return std::unexpected(byte1.error());
        if (!byte2) return std::unexpected(byte2.error());

        if (byte1.value() == Response::eAcknowledge
            && byte2.value() == Response::eDeviceTestPassed)
        {
            auto id1 = TryRead();
            auto id2 = TryRead();

            if (id1) LogTrace("id[0] = {:#x}", id1.value());
            if (id2) LogTrace("id[1] = {:#x}", id2.value());
            return {};
        }

        return std::unexpected(ENODEV);
    }
}; // namespace I8042Controller
