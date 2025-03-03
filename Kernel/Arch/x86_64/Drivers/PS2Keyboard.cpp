/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Arch/InterruptHandler.hpp>
#include <Arch/InterruptManager.hpp>

#include <Arch/x86_64/Drivers/I8042Controller.hpp>
#include <Arch/x86_64/Drivers/PS2Keyboard.hpp>
#include <Arch/x86_64/Drivers/PS2Scancodes.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/TTY.hpp>

#include <cctype>

extern bool g_Decckm;

enum KeyModifier
{
    eModAlt      = 0x01,
    eModControl  = 0x02,
    eModShift    = 0x04,
    eModCapsLock = 0x08,
    eIsPressed   = 0x80,
};

#define SCANCODE_MAX             0x57
#define SCANCODE_CTRL            0x1d
#define SCANCODE_CTRL_REL        0x9d
#define SCANCODE_SHIFT_RIGHT     0x36
#define SCANCODE_SHIFT_RIGHT_REL 0xb6
#define SCANCODE_SHIFT_LEFT      0x2a
#define SCANCODE_SHIFT_LEFT_REL  0xaa
#define SCANCODE_ALT_LEFT        0x38
#define SCANCODE_ALT_LEFT_REL    0xb8
#define SCANCODE_CAPSLOCK        0x3a
#define SCANCODE_NUMLOCK         0x45

static int s_Modifiers = (KeyModifier)0;

void       PS2Keyboard::Initialize()
{
    I8042Controller* controller
        = reinterpret_cast<I8042Controller*>(m_Controller);
    (void)controller->SendCommand(I8042Command::eReadConfigurationByte);
    u8 configuration = controller->ReadBlocking();

    // TODO(v1tr10l7): I think we should disable the port1 translation later
    // on
    configuration |= I8042Configuration::eEnablePort1Irq
                   | I8042Configuration::eEnablePort1Translation;
    configuration &= ~I8042Configuration::eDisablePort1Clock;

    (void)controller->SendCommand(I8042Command::eWriteConfigurationByte);
    controller->WriteBlocking(I8042Port::eBuffer, configuration);

    (void)controller->SendCommand(m_Port == PS2Port::ePort1
                                      ? I8042Command::eEnablePort1
                                      : I8042Command::eEnablePort2);

    auto handler = InterruptManager::AllocateHandler(0x21);
    handler->Reserve();
    handler->SetHandler(HandleInterrupt);
    InterruptManager::Unmask(0x01);

    while (!controller->IsInputEmpty())
        ;

    LogInfo("PS2Keyboard: Installed handler at vector: {}",
            handler->GetInterruptVector());
}

void Emit(const char* ptr, usize count)
{
    TTY* current = TTY::GetCurrent();
    for (usize i = 0; i < count; i++)
    {
        char c = ptr[i];
        current->PutChar(c);
    }
}

void PS2Keyboard::HandleInterrupt(CPUContext* ctx)
{
    bool             extraScancodes = false;

    I8042Controller* controller     = I8042Controller::GetInstance();
    while (!controller->IsOutputEmpty())
    {
        byte raw = controller->ReadBlocking();

        if (raw == 0xe0)
        {
            extraScancodes = true;
            continue;
        }
        if (extraScancodes)
        {
            extraScancodes = false;
            switch (raw)
            {
                case SCANCODE_CTRL: s_Modifiers |= eModControl; continue;
                case SCANCODE_CTRL_REL: s_Modifiers &= ~eModControl; continue;
                case 0x1c: Emit("\n", 1); continue;
                case 0x35: Emit("/", 1); continue;
                case 0x48: Emit("\e[A", 3); continue;
                case 0x4b:
                    if (!g_Decckm) Emit("\e[D", 3);
                    else Emit("\e0D", 3);
                    continue;
                case 0x50:
                    if (!g_Decckm) Emit("\e[B", 3);
                    else Emit("\e0B", 3);
                    continue;
                case 0x4d:
                    if (!g_Decckm) Emit("\e[C", 3);
                    else Emit("\e0C", 3);
                    continue;
                case 0x47: Emit("\e[1~", 4); continue;
                case 0x4f: Emit("\e[4~", 4); continue;
                case 0x49: Emit("\e[5~", 4); continue;
                case 0x51: Emit("\e[6~", 4); continue;
                case 0x53: Emit("\e[3~", 4); continue;
            }
        }

        switch (raw)
        {
            case SCANCODE_NUMLOCK: continue;
            case SCANCODE_ALT_LEFT: continue;
            case SCANCODE_ALT_LEFT_REL: continue;
            case SCANCODE_SHIFT_LEFT:
            case SCANCODE_SHIFT_RIGHT: s_Modifiers |= eModShift; continue;
            case SCANCODE_SHIFT_LEFT_REL:
            case SCANCODE_SHIFT_RIGHT_REL: s_Modifiers &= ~eModShift; continue;

            case SCANCODE_CTRL: s_Modifiers |= eModControl; continue;
            case SCANCODE_CTRL_REL: s_Modifiers &= ~eModControl; continue;
            case SCANCODE_CAPSLOCK:
                if (s_Modifiers & eModCapsLock) s_Modifiers &= ~eModCapsLock;
                else s_Modifiers |= eModCapsLock;
                continue;
        }

        char c = 0;
        if (raw >= SCANCODE_MAX) continue;
        if (!(s_Modifiers & eModCapsLock) && !(s_Modifiers & eModShift))
            c = s_Ps2ScanCodeSet1_Map[raw];
        if (!(s_Modifiers & eModCapsLock) && s_Modifiers & eModShift)
            c = s_Ps2ScanCodeSet1_ShiftMap[raw];
        if (s_Modifiers & eModCapsLock && !(s_Modifiers & eModShift))
            c = s_Ps2ScanCodeSet1_CapsLockMap[raw];
        if (s_Modifiers & eModCapsLock && s_Modifiers & eModShift)
            c = s_Ps2ScanCodeSet1_ShiftCapsLockMap[raw];

        if (s_Modifiers & eModControl) c = std::toupper(c) - 0x40;
        bool pressed = !(raw & 0x80);

        if (!pressed) continue;
        Emit(&c, 1);

// #define KEYBOARD_DEBUG
#ifdef KEYBOARD_DEBUG
        LogDebug("Keyboard::HandleInterrupt: {} {}\n", ch,
                 pressed ? "down" : "up");
#endif
        // TTY* current = TTY::GetCurrent();

        /*
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
        }*/
    }
}
