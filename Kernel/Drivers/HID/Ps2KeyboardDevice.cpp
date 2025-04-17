/*
 * Created by v1tr10l7 on 27.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/PowerManager.hpp>
#include <Boot/CommandLine.hpp>

#include <Drivers/HID/PS2Scancodes.hpp>
#include <Drivers/HID/Ps2KeyboardDevice.hpp>
#include <Drivers/TTY.hpp>

#include <cctype>

constexpr usize SCANCODE_MAX               = std::size(PS2_Set1_Keys);
constexpr usize SCANCODE_CTRL_PRESS        = 0x1d;
constexpr usize SCANCODE_CTRL_REL          = 0x9d;
constexpr usize SCANCODE_SHIFT_RIGHT_PRESS = 0x36;
constexpr usize SCANCODE_SHIFT_RIGHT_REL   = 0xb6;
constexpr usize SCANCODE_SHIFT_LEFT_PRESS  = 0x2a;
constexpr usize SCANCODE_SHIFT_LEFT_REL    = 0xaa;
constexpr usize SCANCODE_ALT_LEFT_PRESS    = 0x38;
constexpr usize SCANCODE_ALT_LEFT_REL      = 0xb8;
constexpr usize SCANCODE_CAPSLOCK_TOGGLE   = 0x3a;
constexpr usize SCANCODE_NUMLOCK           = 0x45;
constexpr usize SCANCODE_ENTER             = 0x1c;
constexpr usize SCANCODE_UP_ARROW          = 0x48;
constexpr usize SCANCODE_DOWN_ARROW        = 0x50;
constexpr usize SCANCODE_LEFT_ARROW        = 0x4b;
constexpr usize SCANCODE_RIGHT_ARROW       = 0x4d;
constexpr usize SCANCODE_HOME_PRESS        = 0x47;
constexpr usize SCANCODE_END_PRESS         = 0x4f;
constexpr usize SCANCODE_PAGE_UP_PRESS     = 0x49;
constexpr usize SCANCODE_PAGE_DOWN_PRESS   = 0x51;
constexpr usize SCANCODE_DELETE_PRESS      = 0x53;

void            Ps2KeyboardDevice::Initialize()
{
    // TODO(v1tr10l7): initialize the keyboard device
    if (!m_Controller->ResetDevice(m_Port))
    {
        LogError("Ps2KeyboardDevice: Failed to reset the device");
        return;
    }
    if (!m_Controller->EnableDevice(m_Port))
    {
        LogError("Ps2KeyboardDevice: Failed to enable the device's port");
        return;
    }
}

void Ps2KeyboardDevice::OnByteReceived(u8 byte)
{
    switch (m_ScanCodeSet)
    {
        case ScanCodeSet::eSet1: HandleScanCodeSet1Key(byte); break;
        case ScanCodeSet::eSet2: HandleScanCodeSet2Key(byte); break;
        case ScanCodeSet::eSet3:

        default: AssertNotReached();
    }
}

void Ps2KeyboardDevice::HandleScanCodeSet1Key(u8 raw)
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
            = CommandLine::GetBoolean("disableArrowKeys").value_or(false);

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
                Emit(cursorKeyMode ? "\eOA" : "\e[A", 3);
                return;
            case SCANCODE_LEFT_ARROW:
                if (disableArrowKeys) return;
                Emit(cursorKeyMode ? "\eOD" : "\e[D", 3);
                return;
            case SCANCODE_DOWN_ARROW:
                if (disableArrowKeys) return;
                Emit(cursorKeyMode ? "\eOB" : "\e[B", 3);
                return;
            case SCANCODE_RIGHT_ARROW:
                if (disableArrowKeys) return;
                Emit(cursorKeyMode ? "\eOC" : "\e[C", 3);
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

    if (m_Modifiers & KeyModifier::eControl) c = std::toupper(c) - 0x40;

    if (!pressed) return;

    if (m_Modifiers & KeyModifier::eShift && m_Modifiers & KeyModifier::eAlt
        && c == '\r')
        PowerManager::Reboot();
    Emit(&c, 1);
}
void Ps2KeyboardDevice::HandleScanCodeSet2Key(u8 raw) {}

void Ps2KeyboardDevice::Emit(const char* str, usize count)
{
    TTY* current = TTY::GetCurrent();
    if (!current) return;

    current->SendBuffer(str, count);
}
