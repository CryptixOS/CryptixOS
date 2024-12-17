/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/PS2Keyboard.hpp>

#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>

#include <Arch/x86_64/Drivers/I8042Controller.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/TTY.hpp>

static char s_Map[0x80]
    = {0,   '\033', '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0',
       '-', '=',    '\b', 0,   'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
       'o', 'p',    '[',  ']', '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
       'j', 'k',    'l',  ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
       'b', 'n',    'm',  ',', '.',  '/', 0,   0,    0,   ' '};

static char s_ShiftMap[0x80]
    = {0,   '\033', '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')',
       '_', '+',    '\b', 0,   'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
       'O', 'P',    '{',  '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
       'J', 'K',    'L',  ':', '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
       'B', 'N',    'M',  '<', '>',  '?', 0,   0,   0,   ' '};

enum KeyModifier
{
    eModAlt     = 0x01,
    eModControl = 0x02,
    eModShift   = 0x04,
    eIsPressed  = 0x80,
};
static int s_Modifiers = (KeyModifier)0;

void       ClearBuffer();
void       HandleInterrupt(CPUContext* ctx);

void       PS2Keyboard::Initialize(PS2Port port)
{
    m_Port = port;
    I8042Controller::SendCommand(I8042Command::eReadConfigurationByte);
    u8 configuration = I8042Controller::ReadBlocking();

    // TODO(v1tr10l7): I think we should disable the port1 translation later
    // on
    configuration |= I8042Configuration::eEnablePort1Irq
                   | I8042Configuration::eEnablePort1Translation;
    configuration &= ~I8042Configuration::eDisablePort1Clock;

    I8042Controller::SendCommand(I8042Command::eWriteConfigurationByte);
    I8042Controller::WriteBlocking(I8042Port::eBuffer, configuration);

    I8042Controller::SendCommand(I8042Command::eEnablePort1);

    auto handler = InterruptManager::AllocateHandler(0x21);
    handler->Reserve();
    handler->SetHandler(HandleInterrupt);
    InterruptManager::Unmask(0x01);

    while (!I8042Controller::IsInputEmpty())
        ;

    LogInfo("PS2Keyboard: Installed handler at vector: {}",
            handler->GetInterruptVector());
}

void PS2Keyboard::HandleInterrupt(CPUContext* ctx)
{
    while (!I8042Controller::IsOutputEmpty())
    {
        byte raw     = I8042Controller::ReadBlocking();
        byte ch      = raw & 0x7f;
        bool pressed = !(raw & 0x80);

// #define KEYBOARD_DEBUG
#ifdef KEYBOARD_DEBUG
        LogDebug("Keyboard::HandleInterrupt: {} {}\n", ch,
                 pressed ? "down" : "up");
#endif
        TTY* current = TTY::GetCurrent();

        switch (ch)
        {
            case 0x38:
                if (pressed) s_Modifiers |= eModAlt;
                else s_Modifiers &= ~eModAlt;
            case 0x1d:
                if (pressed) s_Modifiers |= eModControl;
                else s_Modifiers &= ~eModControl;
            case 0x2a:
                if (pressed) s_Modifiers |= eModShift;
                else s_Modifiers &= ~eModShift;
            case I8042Response::eAcknowledge: break;
            default:

                if (ch & 0x80 || !current | !pressed) break;

                if (!s_Modifiers) current->PutChar(s_Map[ch]);
                if (s_Modifiers & eModShift) current->PutChar(s_ShiftMap[ch]);
                break;
        }
    }
}
