/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "PS2Keyboard.hpp"

#include "Arch/InterruptHandler.hpp"
#include "Arch/InterruptManager.hpp"

#include "Arch/x86_64/IO.hpp"

#include "Drivers/TTY.hpp"

#define I8042_BUFFER   0x60
#define I8042_STATUS   0x64
#define DATA_AVAILABLE 0x01
#define I8042_ACK      0xfa

static char map[0x80]
    = {0,   '\033', '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0',
       '-', '=',    '\b', 0,   'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
       'o', 'p',    '[',  ']', '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
       'j', 'k',    'l',  ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
       'b', 'n',    'm',  ',', '.',  '/', 0,   0,    0,   ' '};

static char shiftMap[0x80]
    = {0,   '\033', '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')',
       '_', '+',    '\b', 0,   'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
       'O', 'P',    '{',  '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
       'J', 'K',    'L',  ':', '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
       'B', 'N',    'M',  '<', '>',  '?', 0,   0,   0,   ' '};

namespace PS2Keyboard
{
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

    void       Initialize()
    {
        ClearBuffer();
        auto handler = InterruptManager::AllocateHandler(0x21);
        handler->Reserve();
        handler->SetHandler(HandleInterrupt);
        InterruptManager::Unmask(0x01);

        while (IO::In<byte>(I8042_STATUS) & Bit(1))
            ;
        IO::Out<byte>(0x64, 0xF3);
        IO::Out<byte>(0x64, 0x00);

        LogInfo("PS2Keyboard: Installed handler at vector: {}",
                handler->GetInterruptVector());
    }

    void HandleInterrupt(CPUContext* ctx)
    {
        while (IO::In<byte>(I8042_STATUS) & DATA_AVAILABLE)
        {
            byte raw     = IO::In<byte>(I8042_BUFFER);
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
                case I8042_ACK: break;
                default:
                    if (!current) break;
                    if (s_Modifiers & eModShift)
                    {
                        current->PutChar(shiftMap[ch]);
                        break;
                    }
                    current->PutChar(map[ch]);
                    break;
            }
        }
    }

    void ClearBuffer()
    {
        while (IO::In<byte>(I8042_STATUS) & DATA_AVAILABLE)
            IO::In<byte>(I8042_BUFFER);
    }
} // namespace PS2Keyboard
