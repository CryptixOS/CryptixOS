/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <AtKeyboard.hpp>
#include <Scancodes.hpp>

#ifdef __x86_64__
    #include <x86_64/input/i8042/I8042.hpp>
#endif

#include <Arch/PowerManager.hpp>
#include <Boot/CommandLine.hpp>
#include <Drivers/TTY/TTY.hpp>
#include <Drivers/TTY/VirtualConsole.hpp>

#include <Prism/Core/Ranges.hpp>
#include <Prism/String/StringUtils.hpp>
using namespace Prism;

constexpr usize   SCANCODE_MAX               = Size(PS2_Set1_Keys);
constexpr usize   SCANCODE_CTRL_PRESS        = 0x1d;
constexpr usize   SCANCODE_CTRL_REL          = 0x9d;
constexpr usize   SCANCODE_SHIFT_RIGHT_PRESS = 0x36;
constexpr usize   SCANCODE_SHIFT_RIGHT_REL   = 0xb6;
constexpr usize   SCANCODE_SHIFT_LEFT_PRESS  = 0x2a;
constexpr usize   SCANCODE_SHIFT_LEFT_REL    = 0xaa;
constexpr usize   SCANCODE_ALT_LEFT_PRESS    = 0x38;
constexpr usize   SCANCODE_ALT_LEFT_REL      = 0xb8;
constexpr usize   SCANCODE_CAPSLOCK_TOGGLE   = 0x3a;
constexpr usize   SCANCODE_NUMLOCK           = 0x45;
constexpr usize   SCANCODE_ENTER             = 0x1c;
constexpr usize   SCANCODE_UP_ARROW          = 0x48;
constexpr usize   SCANCODE_DOWN_ARROW        = 0x50;
constexpr usize   SCANCODE_LEFT_ARROW        = 0x4b;
constexpr usize   SCANCODE_RIGHT_ARROW       = 0x4d;
constexpr usize   SCANCODE_HOME_PRESS        = 0x47;
constexpr usize   SCANCODE_END_PRESS         = 0x4f;
constexpr usize   SCANCODE_PAGE_UP_PRESS     = 0x49;
constexpr usize   SCANCODE_PAGE_DOWN_PRESS   = 0x51;
constexpr usize   SCANCODE_DELETE_PRESS      = 0x53;

extern AtomicBool g_LogSyscalls;

void              AtKeyboard::Initialize()
{
    // TODO(v1tr10l7): initialize the keyboard device
    if (!m_Controller->ResetDevice(m_Port))
    {
        LogError("AtKeyboard: Failed to reset the device");
        return;
    }
    if (!m_Controller->EnableDevice(m_Port))
    {
        LogError("AtKeyboard: Failed to enable the device's port");
        return;
    }
}

void AtKeyboard::OnByteReceived(u8 byte)
{
    switch (m_ScanCodeSet)
    {
        case ScanCodeSet::eSet1: HandleScanCodeSet1Key(byte); break;
        case ScanCodeSet::eSet2: HandleScanCodeSet2Key(byte); break;
        case ScanCodeSet::eSet3:

        default: AssertNotReached();
    }
}

void AtKeyboard::HandleScanCodeSet1Key(u8 raw)
{
    bool pressed = !(raw & 0x80);

    if (raw == 0xe0)
    {
        m_ExtraScanCode = true;
        return;
    }

    if (m_ExtraScanCode)
    {
        m_ExtraScanCode    = false;
        auto tty           = TTY::GetCurrent();
        bool cursorKeyMode = tty ? tty->GetCursorKeyMode() : false;
        bool disableArrowKeys
            = CommandLine::GetBoolean("disableArrowKeys").ValueOr(false);

        char cursorSequence[4];
        cursorSequence[0] = '\e';
        cursorSequence[1] = cursorKeyMode ? 'O' : '[';
        cursorSequence[3] = '\0';

        switch (raw)
        {
            case SCANCODE_CTRL_PRESS:
            case SCANCODE_CTRL_REL:
                if (pressed) m_Modifiers |= KeyModifier::eControl;
                else m_Modifiers &= ~KeyModifier::eControl;
                return;
            case SCANCODE_ENTER: Emit("\n", 1); return;
            case 0x35: Emit("/", 1); return;
            case SCANCODE_UP_ARROW:
                if (disableArrowKeys) return;
                cursorSequence[2] = 'A';
                Emit(cursorSequence, 4);
                LogDebug("atkbd: UP_ARROW => {}", cursorSequence);
                return;
            case SCANCODE_LEFT_ARROW:
                if (disableArrowKeys) return;
                cursorSequence[2] = 'D';
                Emit(cursorSequence, 4);
                LogDebug("atkbd: LEFT_ARROW => {}", cursorSequence);
                return;
            case SCANCODE_DOWN_ARROW:
                if (disableArrowKeys) return;
                cursorSequence[2] = 'B';
                Emit(cursorSequence, 4);
                LogDebug("atkbd: DOWN_ARROW => {}", cursorSequence);
                return;
            case SCANCODE_RIGHT_ARROW:
                if (disableArrowKeys) return;
                cursorSequence[2] = 'C';
                Emit(cursorSequence, 4);
                LogDebug("atkbd: RIGHT_ARROW => {}", cursorSequence);
                return;
            case SCANCODE_HOME_PRESS: Emit("\e[1~", 4); return;
            case SCANCODE_END_PRESS: Emit("\e[4~", 4); return;
            case SCANCODE_PAGE_UP_PRESS: Emit("\e[5~", 4); return;
            case SCANCODE_PAGE_DOWN_PRESS: Emit("\e[6~", 4); return;
            case SCANCODE_DELETE_PRESS: Emit("\0177", 1); return;
        }
    }

    switch (raw)
    {
        case SCANCODE_NUMLOCK: return; // Num Lock
        case SCANCODE_ALT_LEFT_PRESS:
        case SCANCODE_ALT_LEFT_REL:
            if (pressed) m_Modifiers |= KeyModifier::eAlt;
            else m_Modifiers &= ~KeyModifier::eAlt;
            return;
        case SCANCODE_SHIFT_LEFT_PRESS:  // Left Shift Press
        case SCANCODE_SHIFT_RIGHT_PRESS: // Right Shift Press
        case SCANCODE_SHIFT_LEFT_REL:    // Left Shift Release
        case SCANCODE_SHIFT_RIGHT_REL:   // Right Shift Release
            if (pressed) m_Modifiers |= KeyModifier::eShift;
            else m_Modifiers &= ~KeyModifier::eShift;
            return;
        case SCANCODE_CTRL_PRESS:
        case SCANCODE_CTRL_REL:
            if (pressed) m_Modifiers |= KeyModifier::eControl;
            else m_Modifiers &= ~KeyModifier::eControl;
            return;
        case SCANCODE_CAPSLOCK_TOGGLE: // Caps Lock
            if (pressed) m_Modifiers |= KeyModifier::eCapsLock;
            else m_Modifiers &= ~KeyModifier::eCapsLock;
            return;
    }

    char c = 0;

    if (raw >= SCANCODE_MAX) return;
    if (!(m_Modifiers & KeyModifier::eCapsLock)
        && !(m_Modifiers & KeyModifier::eShift))
        c = PS2_Set1_Keys[raw].CodePoint;
    if (!(m_Modifiers & KeyModifier::eCapsLock)
        && m_Modifiers & KeyModifier::eShift)
        c = PS2_Set1_Keys[raw].ShiftCodePoint;
    if (m_Modifiers & KeyModifier::eCapsLock
        && !(m_Modifiers & KeyModifier::eShift))
        c = PS2_Set1_Keys[raw].CapsLockCodePoint;
    if (m_Modifiers & KeyModifier::eCapsLock
        && m_Modifiers & KeyModifier::eShift)
        c = PS2_Set1_Keys[raw].ShiftCapsLockCodePoint;
    CtosUnused(c);

    if (m_Modifiers & KeyModifier::eControl) c = StringUtils::ToUpper(c) - 0x40;

    if (!pressed) return;
    if (m_Modifiers & KeyModifier::eShift && m_Modifiers & KeyModifier::eAlt)
    {
        if (c == '\r') PowerManager::Reboot();
        else if (c >= '!' && c <= '(')
        {
            isize index = c - '!' + 1;
            Assert(index >= 0);
            VirtualConsole::SwitchTo(index - 1);
        }
    }
    else if (m_Modifiers & KeyModifier::eShift
             && m_Modifiers & KeyModifier::eAlt)
        g_LogSyscalls = !g_LogSyscalls;
    Emit(&c, 1);
}
void AtKeyboard::HandleScanCodeSet2Key(u8 raw) {}

void AtKeyboard::Emit(const char* str, usize count)
{
    TTY* current = TTY::GetCurrent();
    if (!current) return;

    current->SendBuffer(str, count);
}

#include <Library/Logger.hpp>
#include <Library/Module.hpp>

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("i8042 atkbd keyboard driver");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");
CTOS_MODULE_SOFTDEP("pre: i8042");

extern void                 printStuff();
extern "C" CTOS_EXPORT bool ModuleInit()
{
    LogInfo("Hello, World from Kernel Module");

#ifdef __x86_64__
    auto ctrl        = I8042::Instance();
    auto scancodeSet = Ps2ScanCodeSet::eSet1;

    Assert(DeviceManager::AllocateCharMajor(API::DeviceMajor::MISCELLANEOUS));
    auto kbd
        = CreateRef<AtKeyboard>(ctrl, SerioDevicePort::ePort1, scancodeSet);
    DeviceManager::RegisterCharDevice(kbd.Raw());

    ctrl->RegisterPort(SerioDevicePort::ePort1, kbd);
#endif
    return true;
}

MODULE_INIT(atkbd, ModuleInit);
