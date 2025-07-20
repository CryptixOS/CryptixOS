/*
 * Created by v1tr10l7 on 05.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/InterruptManager.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Arch/x86_64/Drivers/I8042Controller.hpp>

#include <Drivers/DeviceManager.hpp>
#include <Drivers/HID/Ps2KeyboardDevice.hpp>
#include <Firmware/ACPI/ACPI.hpp>

#include <Prism/Memory/Ref.hpp>
#include <Prism/String/StringUtils.hpp>

Ref<Ps2KeyboardDevice>  s_Keyboard;

Ps2Controller*          Ps2Controller::s_Instance = nullptr;
static I8042Controller* s_Controller              = nullptr;

static Ps2DeviceType    ToPs2DeviceType(Optional<u8> byte1,
                                        Optional<u8> byte2)
{
    if (!byte1 && !byte2) return Ps2DeviceType::eATKeyboard;
    else if (byte1.ValueOr(1) == 0x00 && !byte2)
        return Ps2DeviceType::eStandardMouse;
    else if (byte1.ValueOr(0) == 0x03 && !byte2)
        return Ps2DeviceType::eScrollWheelMouse;
    else if (byte1.ValueOr(0) == 0x04 && !byte2)
        return Ps2DeviceType::e5ButtonMouse;
    else if (byte1.ValueOr(0) == 0xab
             && (byte2.ValueOr(0) == 0x83 || byte2.ValueOr(0) == 0xc1))
        return Ps2DeviceType::eMf2Keyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x84)
        return Ps2DeviceType::eThinkPadKeyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x85)
        return Ps2DeviceType::eNcdKeyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x86)
        return Ps2DeviceType::eStandardKeyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x90)
        return Ps2DeviceType::eJapaneseGKeyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x91)
        return Ps2DeviceType::eJapanesePKeyboard;
    else if (byte1.ValueOr(0) == 0xab && byte2.ValueOr(0) == 0x92)
        return Ps2DeviceType::eJapaneseAKeyboard;
    else if (byte1.ValueOr(0) == 0xac && byte2.ValueOr(0) == 0xa1)
        return Ps2DeviceType::eNcdKeyboard;

    return Ps2DeviceType::eUndefined;
}

ErrorOr<void> I8042Controller::Probe()
{
    LogTrace("I8042: Trying to probe the controller...");
    if (!QuerySupport()) return Error(ENODEV);
    LogTrace(
        "I8042: Detected the controller existence, using FADT table from ACPI");

    s_Controller = new I8042Controller();
    if (!s_Controller) return Error(ENOMEM);

    s_Instance   = s_Controller;
    auto ret     = s_Controller->Initialize();

    auto handler = InterruptManager::AllocateHandler(0x21);
    handler->Reserve();
    handler->SetHandler(I8042Controller::HandleInterrupt);

    InterruptManager::Unmask(0x01);
    return ret;
}

ErrorOr<void> I8042Controller::Initialize()
{
    // TODO(v1tr10l7): Disable USB Legacy Support

    // Sometimes, data can get stuck in the PS/2 controller's output buffer. To
    // guard against this, we flush the output buffer.
    if (!FlushReadBuffer())
    {
        LogError("I8042: Failed to flush the output buffer!");
        return Error(ENODEV);
    }

    // Disable devices, so they won't send the data at the wrong time and mess
    // up the initialization
    if (!DisableDevices()) return Error(ENODEV);

    // We need to save the configuration byte because it can get lost during the
    // controller's self-test.
    if (!SendCommand(Command::eReadConfigurationByte)) return Error(ENODEV);
    u8 config = ReadBlocking();

    // Disable port1 irq and port1 translation
    config &= ~Configuration::eEnablePort1Irq;
    config &= ~Configuration::eEnablePort1Translation;

    // Disable port2 irq
    config &= ~Configuration::eEnablePort2Irq;

    // Make sure port1 clock signal is enabled
    config &= ~Configuration::eDisablePort1Clock;
    if (!SendCommand(Command::eWriteConfigurationByte)) return Error(ENODEV);
    if (!TryWrite(Port::eBuffer, config)) return Error(ENODEV);

    if (!PerformSelfTest())
    {
        LogError("I8042: Controller Self Test failed!");
        return Error(ENODEV);
    }

    m_Port2Available = IsDualChannel();
    if (!TestInterfaces())
    {
        LogError("I8042: Both ports failed to pass self tests");
        return Error(ENODEV);
    }

    LogInfo("I8042: The controller was successfully initialized");
    LogTrace("I8042: Enumerating the available PS/2 devices...");
    EnumerateDevices();

    LogInfo("I8042: Initialized");
    return {};
    auto handler = InterruptManager::AllocateHandler(0x21);
    handler->Reserve();
    handler->SetHandler(I8042Controller::HandleInterrupt);
    InterruptManager::Unmask(0x01);

    while (!IsInputEmpty());

    LogInfo("I8042: Allocated IRQ handler for vector: {:#x}",
            handler->GetInterruptVector());
    return {};
}

bool I8042Controller::IsOutputEmpty()
{
    return (ReadPort(Port::eStatus) & ToUnderlying(Status::eOutBufferFull))
        == 0;
}
bool I8042Controller::IsInputEmpty()
{
    return (ReadPort(Port::eStatus) & ToUnderlying(Status::eInBufferFull)) == 0;
}

u8 I8042Controller::ReadBlocking()
{
    while (IsOutputEmpty()) Arch::Pause();

    return ReadPort(Port::eBuffer);
}
void I8042Controller::WriteBlocking(Port port, u8 data)
{
    while (!IsInputEmpty()) Arch::Pause();

    WritePort(port, data);
}

ErrorOr<u8> I8042Controller::TryRead()
{
    auto status = WaitForIncomingData();
    if (!status) return status.Error();

    return ReadPort(Port::eBuffer);
}
ErrorOr<void> I8042Controller::TryWrite(Port port, u8 data)
{
    auto status = WaitForWriteReady();
    if (!status) return status;

    WritePort(port, data);
    return {};
}

ErrorOr<void> I8042Controller::FlushReadBuffer()
{
    constexpr isize MAX_FLUSH_READ_TRY_COUNT = 16;
    isize           tryCount                 = 0;

    while (!IsOutputEmpty())
    {
        if (tryCount++ >= MAX_FLUSH_READ_TRY_COUNT) return Error(EIO);

        IO::Delay(50);
        CtosUnused(ReadPort(Port::eBuffer));
    }

    return {};
}
ErrorOr<void> I8042Controller::SendCommand(Command command)
{
    if (!TryWrite(Port::eCommand, ToUnderlying(command))) return Error(EBUSY);

    return {};
}

ErrorOr<void> I8042Controller::WriteDevicePort(DevicePort port, byte data)
{

    usize attempts = 0;

    byte  response = 0x00;
    while (attempts++ < 150 || response == Response::eResend)
    {
        auto status = WaitForWriteReady();
        if (!status) return status;

        if (port == DevicePort::ePort2)
        {
            if (!m_Port2Available) return Error(ENODEV);
            if (!SendCommand(Command::eWriteToPort2)) return Error(EBUSY);
        }

        if (!TryWrite(Port::eBuffer, data)) return Error(EBUSY);
        response = ReadBlocking();
    }

    if (attempts >= 150 && response == Response::eResend) return Error(EBUSY);

    WriteBlocking(Port::eBuffer, data);
    return {};
}

ErrorOr<void> I8042Controller::EnableDevice(DevicePort port)
{
    if (port != DevicePort::ePort1 && port != DevicePort::ePort2)
        return Error(EINVAL);
    if (!FlushReadBuffer()) return Error(ENODEV);

    if (!SendCommand(Command::eReadConfigurationByte)) return Error(ENODEV);
    u8 config = ReadBlocking();

    // Disable port1 irq and port1 translation
    config |= (port == DevicePort::ePort1)
                ? Configuration::eEnablePort1Irq
                      | Configuration::eEnablePort1Translation
                : Configuration::eEnablePort2Irq;
    config &= (port == DevicePort::ePort1) ? ~Configuration::eDisablePort1Clock
                                           : ~Configuration::eDisablePort2Clock;

    if (!SendCommand(Command::eWriteConfigurationByte)) return Error(ENODEV);
    WriteBlocking(Port::eBuffer, config);

    if (!SendCommand(port == DevicePort::ePort1 ? Command::eEnablePort1
                                                : Command::eEnablePort2))
        return Error(ENODEV);
    return {};
}
ErrorOr<void> I8042Controller::DisableDevice(DevicePort port)
{
    if (port != DevicePort::ePort1 && port != DevicePort::ePort2)
        return Error(EINVAL);
    if (!FlushReadBuffer()) return Error(ENODEV);

    if (!SendCommand(Command::eReadConfigurationByte)) return Error(ENODEV);
    u8 config = ReadBlocking();

    // Disable port1 irq and port1 translation
    config &= (port == DevicePort::ePort1) ? ~Configuration::eEnablePort1Irq
                                           : ~Configuration::eEnablePort2Irq;
    config |= (port == DevicePort::ePort1) ? Configuration::eDisablePort1Clock
                                           : Configuration::eDisablePort2Clock;

    if (!SendCommand(Command::eWriteConfigurationByte)) return Error(ENODEV);
    WriteBlocking(Port::eBuffer, config);

    SendCommand(port == DevicePort::ePort1 ? Command::eDisablePort1
                                           : Command::eDisablePort2);
    return {};
}

ErrorOr<void> I8042Controller::EnablePort1Translation()
{
    FlushReadBuffer();

    SendCommand(Command::eReadConfigurationByte);
    u8 config = ReadBlocking();

    config |= Configuration::eEnablePort1Translation;
    SendCommand(Command::eWriteConfigurationByte);
    WriteBlocking(Port::eBuffer, config);
    return {};
}
ErrorOr<void> I8042Controller::DisablePort1Translation()
{
    FlushReadBuffer();

    SendCommand(Command::eReadConfigurationByte);
    u8 config = ReadBlocking();

    config &= ~Configuration::eEnablePort1Translation;
    SendCommand(Command::eWriteConfigurationByte);
    WriteBlocking(Port::eBuffer, config);
    return {};
}
ErrorOr<void> I8042Controller::ResetDevice(DevicePort port)
{
    FlushReadBuffer();
    auto status = SendDeviceCommand(port, DeviceCommand::eReset);
    if (!status) return status;

    auto byte1 = TryRead();
    auto byte2 = TryRead();

    if (!byte1) return Error(byte1.Error());
    if (!byte2) return Error(byte2.Error());

    if (byte1.Value() == Response::eAcknowledge
        && byte2.Value() == Response::eDeviceTestPassed)
    {
        auto id1 = TryRead();
        auto id2 = TryRead();

        if (id1) LogTrace("id[0] = {:#x}", id1.Value());
        if (id2) LogTrace("id[1] = {:#x}", id2.Value());
        return {};
    }

    return Error(ENODEV);
}

bool I8042Controller::QuerySupport()
{
    // TODO(v1tr10l7): i8042 is also supported when the ACPI isn't

    auto* fadt = ACPI::GetTable<ACPI::FADT>("FACP");
    if (!fadt)
    {
        LogError(
            "I8042: Failed to query for controller support, FADT was not "
            "found");
        return false;
    }

    auto x86Flags = fadt->X86BootArchitectureFlags;
    if (!x86Flags.I8042Available)
    {
        LogError(
            "I8042: Failed to query for controller support, "
            "flag 'ACPI::FADT->IA32BootArchitectureFlags::e8042' is not "
            "set!");

        return false;
    }

    return true;
}
void I8042Controller::HandleInterrupt(CPUContext* context)
{
    while (!GetInstance()->IsOutputEmpty())
    {
        auto byte = s_Controller->TryRead();
        if (byte) s_Keyboard->OnByteReceived(byte.Value());
    }
}

I8042Controller::I8042Controller() {}

ErrorOr<void> I8042Controller::DisableDevices()
{
    if (!SendCommand(Command::eDisablePort1)) return Error(EBUSY);
    if (!SendCommand(Command::eDisablePort2)) return Error(EBUSY);

    return {};
}

bool I8042Controller::PerformSelfTest()
{
    if (!SendCommand(Command::eReadConfigurationByte)) return false;
    u8    config       = ReadBlocking();

    isize attemptCount = 0;
    while (!SendCommand(Command::eTestController) && attemptCount < 5)
    {
        auto status = TryRead();
        if (status && status.Value() == Response::eSelfTestSuccess) break;
        IO::Delay(50);

        ++attemptCount;
    }

    LogTrace(
        "I8042: Giving up on the controller's self-test, continuing anyway...");
    // On some hardware the self test might reset the device,
    // so we're writing to the configuration byte again
    if (!SendCommand(Command::eWriteConfigurationByte)) return false;
    if (!TryWrite(Port::eBuffer, config)) return false;

    return true;
}
bool I8042Controller::IsDualChannel()
{
    SendCommand(Command::eEnablePort2);

    SendCommand(Command::eReadConfigurationByte);
    u8   config        = ReadBlocking();

    bool isDualChannel = (config & Configuration::eDisablePort2Clock) == 0;
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

bool I8042Controller::TestInterfaces()
{
    LogTrace("I8042: Testing interfaces...");
    m_Port1Available = TestSingleInterface(DevicePort::ePort1);
    m_Port2Available = TestSingleInterface(DevicePort::ePort2);

    return m_Port1Available | m_Port2Available;
}
bool I8042Controller::TestSingleInterface(DevicePort port)
{
    Assert(ToUnderlying(port) <= 2);
    LogTrace("I8042: Testing port #{}...", ToUnderlying(port));
    if (!SendCommand(port == DevicePort::ePort1 ? Command::eTestPort1
                                                : Command::eTestPort2))
        return false;
    Response status = static_cast<Response>(ReadBlocking());

    if (status == Response::ePortTestSuccess)
    {
        LogTrace("I8042: Port #{} is working correctly", ToUnderlying(port));

        return true;
    }

    LogError("I8042: Port #1 error -> {}", ToString(status));
    return false;
}

void I8042Controller::EnumerateDevices()
{
    if (m_Port1Available)
    {
        auto deviceType = ScanPortForDevices(DevicePort::ePort1);
        if (deviceType)
            LogTrace("I8042: DeviceType: {}", ToString(deviceType.Value()));
        LogTrace("I8042: Detected device on port #1");
    }
    if (m_Port2Available)
    {
        auto deviceType = ScanPortForDevices(DevicePort::ePort2);
        if (deviceType)
            LogTrace("I8042: DeviceType: {}", ToString(deviceType.Value()));
        LogTrace("I8042: Detected device on port #2");
    }

    auto scancodeSet = Ps2ScanCodeSet::eSet1;

    s_Keyboard
        = CreateRef<Ps2KeyboardDevice>(this, DevicePort::ePort1, scancodeSet);
    DeviceManager::RegisterCharDevice(s_Keyboard.Raw());
}
ErrorOr<Ps2DeviceType> I8042Controller::ScanPortForDevices(DevicePort port)
{
    Assert(port == DevicePort::ePort1 || port == DevicePort::ePort2);
    SendCommand(port == DevicePort::ePort1 ? Command::eEnablePort1
                                           : Command::eEnablePort2);

    auto status = SendDeviceCommand(port, DeviceCommand::eDisableScanning);
    if (!status) return Error(status.Error());

    auto response = TryRead();
    if (!response) return Error(response.Error());
    if (response.Value() != Response::eAcknowledge) return Error(ENODEV);

    FlushReadBuffer();
    status = SendDeviceCommand(port, DeviceCommand::eIdentify);
    if (!status) return Error(status.Error());

    response = TryRead();
    if (!response) return Error(response.Error());
    if (response.Value() != Response::eAcknowledge) return Error(ENODEV);

    auto byte1 = TryRead();
    auto byte2 = TryRead();

    if (byte1)
        LogInfo("I8042: Device #{} reply[0] = {:#x}", ToUnderlying(port),
                byte1.Value());
    if (byte2)
        LogInfo("I8042: Device #{} reply[1] = {:#x}", ToUnderlying(port),
                byte2.Value());

    FlushReadBuffer();
    status = SendDeviceCommand(port, DeviceCommand::eEnableScanning);
    if (!status)
    {
        LogError("I8042: Failed to enable scanning on device #{}",
                 ToUnderlying(port));

        return Error(status.Error());
    }

    response = TryRead();
    if (!response) return Error(response.Error());
    if (response.Value() != Response::eAcknowledge) return Error(ENODEV);

    if (!SendCommand(port == DevicePort::ePort1 ? Command::eDisablePort1
                                                : Command::eDisablePort2))
        return Error(ENODEV);
    FlushReadBuffer();

    Optional<u8> identify1;
    Optional<u8> identify2;
    if (byte1) identify1 = byte1.Value();
    if (byte2) identify2 = byte2.Value();

    return ToPs2DeviceType(identify1, identify2);
}

ErrorOr<void> I8042Controller::WaitForIncomingData()
{
    for (i32 tryCount = 0; tryCount < READ_WRITE_TIMEOUT; tryCount++)
    {
        if (!IsOutputEmpty()) return {};

        IO::Delay(50);
    }

    return Error(EBUSY);
}
ErrorOr<void> I8042Controller::WaitForWriteReady()
{
    for (i32 tryCount = 0; tryCount < READ_WRITE_TIMEOUT; tryCount++)
    {
        if (IsInputEmpty()) return {};
        IO::Delay(50);
    }

    return Error(EBUSY);
}

u8 I8042Controller::ReadPort(Port port)
{
    return IO::In<byte>(ToUnderlying(port));
}
void I8042Controller::WritePort(Port port, u8 data)
{
    IO::Out<byte>(ToUnderlying(port), data);
}
