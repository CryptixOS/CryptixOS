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

#include <cctype>
/*
static const char s_Map[]
    = {'\0', '\e', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
       '-',  '=',  '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
       'o',  'p',  '[',  ']',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
       'j',  'k',  'l',  ';',  '\'', '`',  '\0', '\\', 'z',  'x', 'c', 'v',
       'b',  'n',  'm',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

static const char s_ShiftMap[]
    = {'\0', '\e', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
       '_',  '+',  '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
       'O',  'P',  '{',  '}',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
       'J',  'K',  'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X', 'C', 'V',
       'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};
*/
[[maybe_unused]] static const char s_CapsLockMap[]
    = {'\0', '\e', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
       '-',  '=',  '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
       'O',  'P',  '[',  ']',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
       'J',  'K',  'L',  ';',  '\'', '`',  '\0', '\\', 'Z',  'X', 'C', 'V',
       'B',  'N',  'M',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

[[maybe_unused]] static const char s_ShiftCapsLockMap[]
    = {'\0', '\e', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
       '_',  '+',  '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
       'o',  'p',  '{',  '}',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
       'j',  'k',  'l',  ':',  '"',  '~',  '\0', '|',  'z',  'x', 'c', 'v',
       'b',  'n',  'm',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};

static char s_Map[0x100]
    = {0,   0,   '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0',
       '-', '=', 0177, 0,   'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
       'o', 'p', '[',  ']', 0,    0,   'a', 's',  'd', 'f', 'g', 'h',
       'j', 'k', 'l',  ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
       'b', 'n', 'm',  ',', '.',  '/', 0,   0,    0,   ' '};

static char s_ShiftMap[0x100] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   0,   0,   ' '};
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
    bool extraScancodes = false;

    while (!I8042Controller::IsOutputEmpty())
    {
        byte raw = I8042Controller::ReadBlocking();

        if (raw == 0xe8)
        {
            extraScancodes = true;
            continue;
        }
        if (extraScancodes)
        {
            /*
            switch (raw)
            {
                case SCANCODE_CTRL: s_Modifiers |= eModControl; continue;
                case SCANCODE_CTRL_REL: s_Modifiers &= ~eModControl; continue;
                case 0x1c: Emit("\n", 1, true); continue;
                case 0x35: Emit("/", 1, true); continue;
                case 0x48:
            }*/
        }
        extraScancodes = false;

        byte ch        = raw & 0x7f;
        bool pressed   = !(raw & 0x80);

// #define KEYBOARD_DEBUG
#ifdef KEYBOARD_DEBUG
        LogDebug("Keyboard::HandleInterrupt: {} {}\n", ch,
                 pressed ? "down" : "up");
#endif
        TTY* current = TTY::GetCurrent();

        switch (ch)
        {
            case 0x38:
            case 0xb8:
                if (pressed) s_Modifiers |= eModAlt;
                else s_Modifiers &= ~eModAlt;
            case 0x1d:
            case 0x9d:
                if (pressed) s_Modifiers |= eModControl;
                else s_Modifiers &= ~eModControl;
            case 0x2a:
            case 0xb6:
                if (pressed) s_Modifiers |= eModShift;
                else s_Modifiers &= ~eModShift;
            case 0x1c: current->PutChar('\n'); break;
            case I8042Response::eAcknowledge: break;
            default:

                if (ch & 0x80 || !current || !pressed || ch == 0x5b) break;

                if (s_Modifiers & eModControl)
                    current->PutChar(std::toupper(s_Map[ch]) - 0x40);
                else if (s_Modifiers & eModShift)
                    current->PutChar(s_ShiftMap[ch]);
                else if (!s_Modifiers) current->PutChar(s_Map[ch]);
                break;
        }
    }
}
