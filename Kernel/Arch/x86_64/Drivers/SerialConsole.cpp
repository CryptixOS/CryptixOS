/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/termios.h>

#include <Arch/x86_64/Drivers/SerialConsole.hpp>
#include <Drivers/Serial.hpp>
#include <System/InterruptManager.hpp>

namespace
{
    // COM1's legacy ISA interrupt line
    constexpr u32   COM1_IRQ = 4;
    // Linux numbers the serial ports 4:64 onwards
    constexpr usize TTYS0_MINOR = 64;
} // namespace

SerialConsole::SerialConsole()
    : TTY("ttyS0", TTYS0_MINOR)
{
}

SerialConsole* SerialConsole::Create()
{
    auto console          = new SerialConsole();
    console->m_Dispatcher = InterruptManager::AllocateHandler(
        COM1_IRQ, console, "serial");
    if (!console->m_Dispatcher)
    {
        LogError("SerialConsole: Failed to allocate an interrupt for COM1");
        delete console;
        return nullptr;
    }

    console->m_Dispatcher->SetHandler(HandleInterrupt);
    Serial::EnableReceiveInterrupt();
    console->m_Dispatcher->Unmask();

    LogInfo("SerialConsole: /dev/ttyS0 receives on irq line #{:#x}",
            console->m_Dispatcher->IrqLine());
    return console;
}

void SerialConsole::Put(char c)
{
    // ONLCR: a serial terminal needs "\r\n" to start the next line at
    // column 0
    if (c == '\n' && (GetTermios().c_oflag & OPOST)
        && (GetTermios().c_oflag & ONLCR))
        Serial::Write('\r');
    Serial::Write(c);
}

ErrorOr<void> SerialConsole::TransmitChar(u64 c)
{
    Put(static_cast<char>(c));
    return {};
}
ErrorOr<isize> SerialConsole::Transmit(StringView data)
{
    for (char c : data) Put(c);
    return static_cast<isize>(data.Size());
}

IrqResult SerialConsole::HandleInterrupt(::Device* device, ExecutionContext*)
{
    auto console = static_cast<SerialConsole*>(device);

    // Drain the FIFO: one interrupt can stand for up to 14 bytes
    char  buffer[16];
    usize count = 0;
    for (i32 byte = Serial::TryRead(); byte >= 0; byte = Serial::TryRead())
    {
        buffer[count++] = static_cast<char>(byte);
        if (count == sizeof(buffer))
        {
            console->SendBuffer(buffer, count);
            count = 0;
        }
    }
    if (count) console->SendBuffer(buffer, count);

    return IrqResult::eHandled;
}
