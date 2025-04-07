/*
 * Created by v1tr10l7 on 27.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr const u8 s_Ps2ScanCodeSet1_Map[256] = {
    /* 0x00 */ 0x00, // Undefined
    /* 0x01 */ '\e', // Esc
    /* 0x02 */ '1',
    /* 0x03 */ '2',
    /* 0x04 */ '3',
    /* 0x05 */ '4',
    /* 0x06 */ '5',
    /* 0x07 */ '6',
    /* 0x08 */ '7',
    /* 0x09 */ '8',
    /* 0x0A */ '9',
    /* 0x0B */ '0',
    /* 0x0C */ '-',
    /* 0x0D */ '=',
    /* 0x0E */ '\b', // Backspace
    /* 0x0F */ '\t', // Tab
    /* 0x10 */ 'q',
    /* 0x11 */ 'w',
    /* 0x12 */ 'e',
    /* 0x13 */ 'r',
    /* 0x14 */ 't',
    /* 0x15 */ 'y',
    /* 0x16 */ 'u',
    /* 0x17 */ 'i',
    /* 0x18 */ 'o',
    /* 0x19 */ 'p',
    /* 0x1A */ '[',
    /* 0x1B */ ']',
    /* 0x1C */ '\r', // Enter
    /* 0x1D */ 0x00, // Left Ctrl
    /* 0x1E */ 'a',
    /* 0x1F */ 's',
    /* 0x20 */ 'd',
    /* 0x21 */ 'f',
    /* 0x22 */ 'g',
    /* 0x23 */ 'h',
    /* 0x24 */ 'j',
    /* 0x25 */ 'k',
    /* 0x26 */ 'l',
    /* 0x27 */ ';',
    /* 0x28 */ '\'',
    /* 0x29 */ '`',
    /* 0x2A */ 0x00, // Left Shift
    /* 0x2B */ '\\',
    /* 0x2C */ 'z',
    /* 0x2D */ 'x',
    /* 0x2E */ 'c',
    /* 0x2F */ 'v',
    /* 0x30 */ 'b',
    /* 0x31 */ 'n',
    /* 0x32 */ 'm',
    /* 0x33 */ ',',
    /* 0x34 */ '.',
    /* 0x35 */ '/',
    /* 0x36 */ 0x00, // Right Shift
    /* 0x37 */ '*',  // Keypad *
    /* 0x38 */ 0x00, // Left Alt
    /* 0x39 */ ' ',  // Space
    /* 0x3A */ 0x00, // Caps Lock
    /* 0x3B */ 0x00, // F1
    /* 0x3C */ 0x00, // F2
    /* 0x3D */ 0x00, // F3
    /* 0x3E */ 0x00, // F4
    /* 0x3F */ 0x00, // F5
    /* 0x40 */ 0x00, // F6
    /* 0x41 */ 0x00, // F7
    /* 0x42 */ 0x00, // F8
    /* 0x43 */ 0x00, // F9
    /* 0x44 */ 0x00, // F10
    /* 0x45 */ 0x00, // Num Lock
    /* 0x46 */ 0x00, // Scroll Lock
    /* 0x47 */ '7',  // Keypad 7
    /* 0x48 */ '8',  // Keypad 8
    /* 0x49 */ '9',  // Keypad 9
    /* 0x4A */ '-',
    /* 0x4B */ '4', // Keypad 4
    /* 0x4C */ '5', // Keypad 5
    /* 0x4D */ '6', // Keypad 6
    /* 0x4E */ '+',
    /* 0x4F */ '1', // Keypad 1
    /* 0x50 */ '2', // Keypad 2
    /* 0x51 */ '3', // Keypad 3
    /* 0x52 */ '0', // Keypad 0
    /* 0x53 */ '.',
    /* 0x54 - 0x56 */ 0x00,
    0x00,
    0x00,            // Unused
    /* 0x57 */ 0x00, // F11
    /* 0x58 */ 0x00, // F12
};
constexpr const u8 s_Ps2ScanCodeSet1_ShiftMap[256] = {
    /* 0x00 */ 0x00, // Undefined
    /* 0x01 */ 0x1B, // Esc
    /* 0x02 */ '!',  // Shift + 1
    /* 0x03 */ '@',  // Shift + 2
    /* 0x04 */ '#',  // Shift + 3
    /* 0x05 */ '$',  // Shift + 4
    /* 0x06 */ '%',  // Shift + 5
    /* 0x07 */ '^',  // Shift + 6
    /* 0x08 */ '&',  // Shift + 7
    /* 0x09 */ '*',  // Shift + 8
    /* 0x0A */ '(',  // Shift + 9
    /* 0x0B */ ')',  // Shift + 0
    /* 0x0C */ '_',  // Shift + -
    /* 0x0D */ '+',  // Shift + =
    /* 0x0E */ '\b', // Backspace
    /* 0x0F */ '\t', // Tab
    /* 0x10 */ 'Q',
    /* 0x11 */ 'W',
    /* 0x12 */ 'E',
    /* 0x13 */ 'R',
    /* 0x14 */ 'T',
    /* 0x15 */ 'Y',
    /* 0x16 */ 'U',
    /* 0x17 */ 'I',
    /* 0x18 */ 'O',
    /* 0x19 */ 'P',
    /* 0x1A */ '{',  // Shift + [
    /* 0x1B */ '}',  // Shift + ]
    /* 0x1C */ '\r', // Enter
    /* 0x1D */ 0x00, // Left Ctrl
    /* 0x1E */ 'A',
    /* 0x1F */ 'S',
    /* 0x20 */ 'D',
    /* 0x21 */ 'F',
    /* 0x22 */ 'G',
    /* 0x23 */ 'H',
    /* 0x24 */ 'J',
    /* 0x25 */ 'K',
    /* 0x26 */ 'L',
    /* 0x27 */ ':',  // Shift + ;
    /* 0x28 */ '"',  // Shift + '
    /* 0x29 */ '~',  // Shift + `
    /* 0x2A */ 0x00, // Left Shift
    /* 0x2B */ '|',  // Shift + '/'
    /* 0x2C */ 'Z',
    /* 0x2D */ 'X',
    /* 0x2E */ 'C',
    /* 0x2F */ 'V',
    /* 0x30 */ 'B',
    /* 0x31 */ 'N',
    /* 0x32 */ 'M',
    /* 0x33 */ '<',  // Shift + ,
    /* 0x34 */ '>',  // Shift + .
    /* 0x35 */ '?',  // Shift + /
    /* 0x36 */ 0x00, // Right Shift
    /* 0x37 */ '*',  // Keypad *
    /* 0x38 */ 0x00, // Left Alt
    /* 0x39 */ ' ',  // Space
    /* 0x3A */ 0x00, // Caps Lock
    /* 0x3B */ 0x00, // F1
    /* 0x3C */ 0x00, // F2
    /* 0x3D */ 0x00, // F3
    /* 0x3E */ 0x00, // F4
    /* 0x3F */ 0x00, // F5
    /* 0x40 */ 0x00, // F6
    /* 0x41 */ 0x00, // F7
    /* 0x42 */ 0x00, // F8
    /* 0x43 */ 0x00, // F9
    /* 0x44 */ 0x00, // F10
    /* 0x45 */ 0x00, // Num Lock
    /* 0x46 */ 0x00, // Scroll Lock
    /* 0x47 */ '7',  // Keypad 7
    /* 0x48 */ '8',  // Keypad 8
    /* 0x49 */ '9',  // Keypad 9
    /* 0x4A */ '-',
    /* 0x4B */ '4', // Keypad 4
    /* 0x4C */ '5', // Keypad 5
    /* 0x4D */ '6', // Keypad 6
    /* 0x4E */ '+',
    /* 0x4F */ '1', // Keypad 1
    /* 0x50 */ '2', // Keypad 2
    /* 0x51 */ '3', // Keypad 3
    /* 0x52 */ '0', // Keypad 0
    /* 0x53 */ '.',
    /* 0x54 - 0x56 */ 0x00,
    0x00,
    0x00,            // Unused
    /* 0x57 */ 0x00, // F11
    /* 0x58 */ 0x00, // F12
};
constexpr const u8 s_Ps2ScanCodeSet1_CapsLockMap[256] = {
    /* 0x00 */ 0x00, // Undefined
    /* 0x01 */ 0x1B, // Esc
    /* 0x02 */ '1',  // Numbers remain the same
    /* 0x03 */ '2',
    /* 0x04 */ '3',
    /* 0x05 */ '4',
    /* 0x06 */ '5',
    /* 0x07 */ '6',
    /* 0x08 */ '7',
    /* 0x09 */ '8',
    /* 0x0A */ '9',
    /* 0x0B */ '0',
    /* 0x0C */ '-',
    /* 0x0D */ '=',
    /* 0x0E */ '\b', // Backspace
    /* 0x0F */ '\t', // Tab
    /* 0x10 */ 'Q',  // Letters become uppercase
    /* 0x11 */ 'W',
    /* 0x12 */ 'E',
    /* 0x13 */ 'R',
    /* 0x14 */ 'T',
    /* 0x15 */ 'Y',
    /* 0x16 */ 'U',
    /* 0x17 */ 'I',
    /* 0x18 */ 'O',
    /* 0x19 */ 'P',
    /* 0x1A */ '[',
    /* 0x1B */ ']',
    /* 0x1C */ '\r', // Enter
    /* 0x1D */ 0x00, // Left Ctrl
    /* 0x1E */ 'A',
    /* 0x1F */ 'S',
    /* 0x20 */ 'D',
    /* 0x21 */ 'F',
    /* 0x22 */ 'G',
    /* 0x23 */ 'H',
    /* 0x24 */ 'J',
    /* 0x25 */ 'K',
    /* 0x26 */ 'L',
    /* 0x27 */ ';',
    /* 0x28 */ '\'',
    /* 0x29 */ '`',
    /* 0x2A */ 0x00, // Left Shift
    /* 0x2B */ '\\',
    /* 0x2C */ 'Z',
    /* 0x2D */ 'X',
    /* 0x2E */ 'C',
    /* 0x2F */ 'V',
    /* 0x30 */ 'B',
    /* 0x31 */ 'N',
    /* 0x32 */ 'M',
    /* 0x33 */ ',',
    /* 0x34 */ '.',
    /* 0x35 */ '/',
    /* 0x36 */ 0x00, // Right Shift
    /* 0x37 */ '*',  // Keypad *
    /* 0x38 */ 0x00, // Left Alt
    /* 0x39 */ ' ',  // Space
    /* 0x3A */ 0x00, // Caps Lock
    /* 0x3B */ 0x00, // F1
    /* 0x3C */ 0x00, // F2
    /* 0x3D */ 0x00, // F3
    /* 0x3E */ 0x00, // F4
    /* 0x3F */ 0x00, // F5
    /* 0x40 */ 0x00, // F6
    /* 0x41 */ 0x00, // F7
    /* 0x42 */ 0x00, // F8
    /* 0x43 */ 0x00, // F9
    /* 0x44 */ 0x00, // F10
    /* 0x45 */ 0x00, // Num Lock
    /* 0x46 */ 0x00, // Scroll Lock
    /* 0x47 */ '7',  // Keypad 7
    /* 0x48 */ '8',  // Keypad 8
    /* 0x49 */ '9',  // Keypad 9
    /* 0x4A */ '-',
    /* 0x4B */ '4', // Keypad 4
    /* 0x4C */ '5', // Keypad 5
    /* 0x4D */ '6', // Keypad 6
    /* 0x4E */ '+',
    /* 0x4F */ '1', // Keypad 1
    /* 0x50 */ '2', // Keypad 2
    /* 0x51 */ '3', // Keypad 3
    /* 0x52 */ '0', // Keypad 0
    /* 0x53 */ '.',
    /* 0x54 - 0x56 */ 0x00,
    0x00,
    0x00,            // Unused
    /* 0x57 */ 0x00, // F11
    /* 0x58 */ 0x00, // F12
};
constexpr const u8 s_Ps2ScanCodeSet1_ShiftCapsLockMap[256] = {
    /* 0x00 */ 0x00, // Undefined
    /* 0x01 */ 0x1B, // Esc
    /* 0x02 */ '!',  // Shift + Caps Lock + 1
    /* 0x03 */ '@',  // Shift + Caps Lock + 2
    /* 0x04 */ '#',  // Shift + Caps Lock + 3
    /* 0x05 */ '$',  // Shift + Caps Lock + 4
    /* 0x06 */ '%',  // Shift + Caps Lock + 5
    /* 0x07 */ '^',  // Shift + Caps Lock + 6
    /* 0x08 */ '&',  // Shift + Caps Lock + 7
    /* 0x09 */ '*',  // Shift + Caps Lock + 8
    /* 0x0A */ '(',  // Shift + Caps Lock + 9
    /* 0x0B */ ')',  // Shift + Caps Lock + 0
    /* 0x0C */ '_',  // Shift + Caps Lock + -
    /* 0x0D */ '+',  // Shift + Caps Lock + =
    /* 0x0E */ '\b', // Backspace
    /* 0x0F */ '\t', // Tab
    /* 0x10 */ 'q',  // Letters become lowercase
    /* 0x11 */ 'w',
    /* 0x12 */ 'e',
    /* 0x13 */ 'r',
    /* 0x14 */ 't',
    /* 0x15 */ 'y',
    /* 0x16 */ 'u',
    /* 0x17 */ 'i',
    /* 0x18 */ 'o',
    /* 0x19 */ 'p',
    /* 0x1A */ '{',  // Shift + Caps Lock + [
    /* 0x1B */ '}',  // Shift + Caps Lock + ]
    /* 0x1C */ '\r', // Enter
    /* 0x1D */ 0x00, // Left Ctrl
    /* 0x1E */ 'a',
    /* 0x1F */ 's',
    /* 0x20 */ 'd',
    /* 0x21 */ 'f',
    /* 0x22 */ 'g',
    /* 0x23 */ 'h',
    /* 0x24 */ 'j',
    /* 0x25 */ 'k',
    /* 0x26 */ 'l',
    /* 0x27 */ ':',  // Shift + Caps Lock + ;
    /* 0x28 */ '"',  // Shift + Caps Lock + '
    /* 0x29 */ '~',  // Shift + Caps Lock + `
    /* 0x2A */ 0x00, // Left Shift
    /* 0x2B */ '|',  // Shift + Caps Lock + '\'
    /* 0x2C */ 'z',
    /* 0x2D */ 'x',
    /* 0x2E */ 'c',
    /* 0x2F */ 'v',
    /* 0x30 */ 'b',
    /* 0x31 */ 'n',
    /* 0x32 */ 'm',
    /* 0x33 */ '<',  // Shift + Caps Lock + ,
    /* 0x34 */ '>',  // Shift + Caps Lock + .
    /* 0x35 */ '?',  // Shift + Caps Lock + /
    /* 0x36 */ 0x00, // Right Shift
    /* 0x37 */ '*',  // Keypad *
    /* 0x38 */ 0x00, // Left Alt
    /* 0x39 */ ' ',  // Space
    /* 0x3A */ 0x00, // Caps Lock
    /* 0x3B */ 0x00, // F1
    /* 0x3C */ 0x00, // F2
    /* 0x3D */ 0x00, // F3
    /* 0x3E */ 0x00, // F4
    /* 0x3F */ 0x00, // F5
    /* 0x40 */ 0x00, // F6
    /* 0x41 */ 0x00, // F7
    /* 0x42 */ 0x00, // F8
    /* 0x43 */ 0x00, // F9
    /* 0x44 */ 0x00, // F10
    /* 0x45 */ 0x00, // Num Lock
    /* 0x46 */ 0x00, // Scroll Lock
    /* 0x47 */ '7',  // Keypad 7
    /* 0x48 */ '8',  // Keypad 8
    /* 0x49 */ '9',  // Keypad 9
    /* 0x4A */ '-',
    /* 0x4B */ '4', // Keypad 4
    /* 0x4C */ '5', // Keypad 5
    /* 0x4D */ '6', // Keypad 6
    /* 0x4E */ '+',
    /* 0x4F */ '1', // Keypad 1
    /* 0x50 */ '2', // Keypad 2
    /* 0x51 */ '3', // Keypad 3
    /* 0x52 */ '0', // Keypad 0
    /* 0x53 */ '.',
    /* 0x54 - 0x56 */ 0x00,
    0x00,
    0x00,            // Unused
    /* 0x57 */ 0x00, // F11
    /* 0x58 */ 0x00, // F12
};

enum class KeyCode : i16
{
    eUnknown = 0,
    eNone,
    eNum0,
    eNum1,
    eNum2,
    eNum3,
    eNum4,
    eNum5,
    eNum6,
    eNum7,
    eNum8,
    eNum9,
    eA,
    eB,
    eC,
    eD,
    eE,
    eF,
    eG,
    eH,
    eI,
    eJ,
    eK,
    eL,
    eM,
    eN,
    eO,
    eP,
    eQ,
    eR,
    eS,
    eT,
    eU,
    eV,
    eW,
    eX,
    eY,
    eZ,
    eTilde,
    eF1,
    eF2,
    eF3,
    eF4,
    eF5,
    eF6,
    eF7,
    eF8,
    eF9,
    eF10,
    eF11,
    eF12,
    eF13,
    eF14,
    eF15,
    eEscape,
    eBackspace,
    eTab,
    eCapsLock,
    eEnter,
    eReturn,
    eLShift,
    eRShift,
    eLCtrl,
    eRCtrl,
    eLAlt,
    eRAlt,
    eLSystem,
    eRSystem,
    eSpace,
    eHyphen,
    eEqual,
    eDecimal,
    eLBracket,
    eRBracket,
    eSemicolon,
    eApostrophe,
    eComma,
    ePeriod,
    eSlash,
    eBackslash,
    eUp,
    eDown,
    eLeft,
    eRight,
    eNumpad0,
    eNumpad1,
    eNumpad2,
    eNumpad3,
    eNumpad4,
    eNumpad5,
    eNumpad6,
    eNumpad7,
    eNumpad8,
    eNumpad9,
    eSeparator,
    eAdd,
    eSubtract,
    eMultiply,
    eDivide,
    eInsert,
    eDelete,
    ePageUp,
    ePageDown,
    eHome,
    eEnd,
    eScrollLock,
    eNumLock,
    ePrintScreen,
    ePause,
    eMenu,

    eKeyCount,
};

struct Key
{
    KeyCode Key;
    u32     CodePoint;
    u32     ShiftCodePoint;
    u32     CapsLockCodePoint;
    u32     ShiftCapsLockCodePoint;
};

constexpr Key PS2_Set1_Keys[] = {
    {KeyCode::eNone, 0, 0, 0, 0}, // No key
    {KeyCode::eEscape, '\e', '\e', '\e', '\e'},
    {KeyCode::eNum1, '1', '!', '1', '!'},
    {KeyCode::eNum2, '2', '@', '2', '@'},
    {KeyCode::eNum3, '3', '#', '3', '#'},
    {KeyCode::eNum4, '4', '$', '4', '$'},
    {KeyCode::eNum5, '5', '%', '5', '%'},
    {KeyCode::eNum6, '6', '^', '6', '^'},
    {KeyCode::eNum7, '7', '&', '7', '&'},
    {KeyCode::eNum8, '8', '*', '8', '*'},
    {KeyCode::eNum9, '9', '(', '9', '('},
    {KeyCode::eNum0, '0', ')', '0', ')'},
    {KeyCode::eHyphen, '-', '_', '-', '_'},
    {KeyCode::eEqual, '=', '+', '=', '+'},
    {KeyCode::eBackspace, '\b', '\b', '\b', '\b'},
    {KeyCode::eTab, '\t', '\t', '\t', '\t'},
    {KeyCode::eQ, 'q', 'Q', 'Q', 'q'},
    {KeyCode::eW, 'w', 'W', 'W', 'w'},
    {KeyCode::eE, 'e', 'E', 'E', 'e'},
    {KeyCode::eR, 'r', 'R', 'R', 'r'},
    {KeyCode::eT, 't', 'T', 'T', 't'},
    {KeyCode::eY, 'y', 'Y', 'Y', 'y'},
    {KeyCode::eU, 'u', 'U', 'U', 'u'},
    {KeyCode::eI, 'i', 'I', 'I', 'i'},
    {KeyCode::eO, 'o', 'O', 'O', 'o'},
    {KeyCode::eP, 'p', 'P', 'P', 'p'},
    {KeyCode::eLBracket, '[', '{', '[', '{'},
    {KeyCode::eRBracket, ']', '}', ']', '}'},
    {KeyCode::eEnter, '\r', '\r', '\r', '\r'},
    {KeyCode::eLCtrl, 0, 0, 0, 0},
    {KeyCode::eA, 'a', 'A', 'A', 'a'},
    {KeyCode::eS, 's', 'S', 'S', 's'},
    {KeyCode::eD, 'd', 'D', 'D', 'd'},
    {KeyCode::eF, 'f', 'F', 'F', 'f'},
    {KeyCode::eG, 'g', 'G', 'G', 'g'},
    {KeyCode::eH, 'h', 'H', 'H', 'h'},
    {KeyCode::eJ, 'j', 'J', 'J', 'j'},
    {KeyCode::eK, 'k', 'K', 'K', 'k'},
    {KeyCode::eL, 'l', 'L', 'L', 'l'},
    {KeyCode::eSemicolon, ';', ':', ';', ':'},
    {KeyCode::eApostrophe, '\'', '"', '\'', '"'},
    {KeyCode::eTilde, '`', '~', '`', '~'},
    {KeyCode::eLShift, 0, 0, 0, 0},
    {KeyCode::eBackslash, '\\', '|', '\\', '|'},
    {KeyCode::eZ, 'z', 'Z', 'Z', 'z'},
    {KeyCode::eX, 'x', 'X', 'X', 'x'},
    {KeyCode::eC, 'c', 'C', 'C', 'c'},
    {KeyCode::eV, 'v', 'V', 'V', 'v'},
    {KeyCode::eB, 'b', 'B', 'B', 'b'},
    {KeyCode::eN, 'n', 'N', 'N', 'n'},
    {KeyCode::eM, 'm', 'M', 'M', 'm'},
    {KeyCode::eComma, ',', '<', ',', '<'},
    {KeyCode::ePeriod, '.', '>', '.', '>'},
    {KeyCode::eSlash, '/', '?', '/', '?'},
    {KeyCode::eRShift, 0, 0, 0, 0},
    {KeyCode::eMultiply, '*', '*', '*', '*'},
    {KeyCode::eLAlt, 0, 0, 0, 0},
    {KeyCode::eSpace, ' ', ' ', ' ', ' '},
    {KeyCode::eCapsLock, 0, 0, 0, 0},
    {KeyCode::eF1, 0, 0, 0, 0},
    {KeyCode::eF2, 0, 0, 0, 0},
    {KeyCode::eF3, 0, 0, 0, 0},
    {KeyCode::eF4, 0, 0, 0, 0},
    {KeyCode::eF5, 0, 0, 0, 0},
    {KeyCode::eF6, 0, 0, 0, 0},
    {KeyCode::eF7, 0, 0, 0, 0},
    {KeyCode::eF8, 0, 0, 0, 0},
    {KeyCode::eF9, 0, 0, 0, 0},
    {KeyCode::eF10, 0, 0, 0, 0},
    {KeyCode::eNumLock, 0, 0, 0, 0},
    {KeyCode::eScrollLock, 0, 0, 0, 0},
    {KeyCode::eF11, 0, 0, 0, 0},
    {KeyCode::eF12, 0, 0, 0, 0},
};
