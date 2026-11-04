/*
 * Created by v1tr10l7 on 19.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

/*
 * Device properties and quirks
 */
/* needs a pointer */
constexpr usize INPUT_PROP_POINTER           = 0x00;
/* direct input devices */
constexpr usize INPUT_PROP_DIRECT            = 0x01;
/* has button(s) under pad */
constexpr usize INPUT_PROP_BUTTONPAD         = 0x02;
/* touch rectangle only */
constexpr usize INPUT_PROP_SEMI_MT           = 0x03;
/* softbuttons at top of pad */
constexpr usize INPUT_PROP_TOPBUTTONPAD      = 0x04;
/* is a pointing stick */
constexpr usize INPUT_PROP_POINTING_STICK    = 0x05;
/* has accelerometer */
constexpr usize INPUT_PROP_ACCELEROMETER     = 0x06;

constexpr usize INPUT_PROP_MAX               = 0x1f;
constexpr usize INPUT_PROP_CNT               = (INPUT_PROP_MAX + 1);

/*
 * Event types
 */
constexpr usize EV_SYN                       = 0x00;
constexpr usize EV_KEY                       = 0x01;
constexpr usize EV_REL                       = 0x02;
constexpr usize EV_ABS                       = 0x03;
constexpr usize EV_MSC                       = 0x04;
constexpr usize EV_SW                        = 0x05;
constexpr usize EV_LED                       = 0x11;
constexpr usize EV_SND                       = 0x12;
constexpr usize EV_REP                       = 0x14;
constexpr usize EV_FF                        = 0x15;
constexpr usize EV_PWR                       = 0x16;
constexpr usize EV_FF_STATUS                 = 0x17;
constexpr usize EV_MAX                       = 0x1f;
constexpr usize EV_CNT                       = (EV_MAX + 1);

/*
 * Synchronization events.
 */
constexpr usize SYN_REPORT                   = 0;
constexpr usize SYN_CONFIG                   = 1;
constexpr usize SYN_MT_REPORT                = 2;
constexpr usize SYN_DROPPED                  = 3;
constexpr usize SYN_MAX                      = 0xf;
constexpr usize SYN_CNT                      = (SYN_MAX + 1);

/*
 * Keys and buttons
 *
 * Most of the keys/buttons are modeled after USB HUT 1.12
 * (see http://www.usb.org/developers/hidpage).
 * Abbreviations in the comments:
 * AC - Application Control
 * AL - Application Launch Button
 * SC - System Control
 */
constexpr usize KEY_RESERVED                 = 0;
constexpr usize KEY_ESC                      = 1;
constexpr usize KEY_1                        = 2;
constexpr usize KEY_2                        = 3;
constexpr usize KEY_3                        = 4;
constexpr usize KEY_4                        = 5;
constexpr usize KEY_5                        = 6;
constexpr usize KEY_6                        = 7;
constexpr usize KEY_7                        = 8;
constexpr usize KEY_8                        = 9;
constexpr usize KEY_9                        = 10;
constexpr usize KEY_0                        = 11;
constexpr usize KEY_MINUS                    = 12;
constexpr usize KEY_EQUAL                    = 13;
constexpr usize KEY_BACKSPACE                = 14;
constexpr usize KEY_TAB                      = 15;
constexpr usize KEY_Q                        = 16;
constexpr usize KEY_W                        = 17;
constexpr usize KEY_E                        = 18;
constexpr usize KEY_R                        = 19;
constexpr usize KEY_T                        = 20;
constexpr usize KEY_Y                        = 21;
constexpr usize KEY_U                        = 22;
constexpr usize KEY_I                        = 23;
constexpr usize KEY_O                        = 24;
constexpr usize KEY_P                        = 25;
constexpr usize KEY_LEFTBRACE                = 26;
constexpr usize KEY_RIGHTBRACE               = 27;
constexpr usize KEY_ENTER                    = 28;
constexpr usize KEY_LEFTCTRL                 = 29;
constexpr usize KEY_A                        = 30;
constexpr usize KEY_S                        = 31;
constexpr usize KEY_D                        = 32;
constexpr usize KEY_F                        = 33;
constexpr usize KEY_G                        = 34;
constexpr usize KEY_H                        = 35;
constexpr usize KEY_J                        = 36;
constexpr usize KEY_K                        = 37;
constexpr usize KEY_L                        = 38;
constexpr usize KEY_SEMICOLON                = 39;
constexpr usize KEY_APOSTROPHE               = 40;
constexpr usize KEY_GRAVE                    = 41;
constexpr usize KEY_LEFTSHIFT                = 42;
constexpr usize KEY_BACKSLASH                = 43;
constexpr usize KEY_Z                        = 44;
constexpr usize KEY_X                        = 45;
constexpr usize KEY_C                        = 46;
constexpr usize KEY_V                        = 47;
constexpr usize KEY_B                        = 48;
constexpr usize KEY_N                        = 49;
constexpr usize KEY_M                        = 50;
constexpr usize KEY_COMMA                    = 51;
constexpr usize KEY_DOT                      = 52;
constexpr usize KEY_SLASH                    = 53;
constexpr usize KEY_RIGHTSHIFT               = 54;
constexpr usize KEY_KPASTERISK               = 55;
constexpr usize KEY_LEFTALT                  = 56;
constexpr usize KEY_SPACE                    = 57;
constexpr usize KEY_CAPSLOCK                 = 58;
constexpr usize KEY_F1                       = 59;
constexpr usize KEY_F2                       = 60;
constexpr usize KEY_F3                       = 61;
constexpr usize KEY_F4                       = 62;
constexpr usize KEY_F5                       = 63;
constexpr usize KEY_F6                       = 64;
constexpr usize KEY_F7                       = 65;
constexpr usize KEY_F8                       = 66;
constexpr usize KEY_F9                       = 67;
constexpr usize KEY_F10                      = 68;
constexpr usize KEY_NUMLOCK                  = 69;
constexpr usize KEY_SCROLLLOCK               = 70;
constexpr usize KEY_KP7                      = 71;
constexpr usize KEY_KP8                      = 72;
constexpr usize KEY_KP9                      = 73;
constexpr usize KEY_KPMINUS                  = 74;
constexpr usize KEY_KP4                      = 75;
constexpr usize KEY_KP5                      = 76;
constexpr usize KEY_KP6                      = 77;
constexpr usize KEY_KPPLUS                   = 78;
constexpr usize KEY_KP1                      = 79;
constexpr usize KEY_KP2                      = 80;
constexpr usize KEY_KP3                      = 81;
constexpr usize KEY_KP0                      = 82;
constexpr usize KEY_KPDOT                    = 83;

constexpr usize KEY_ZENKAKUHANKAKU           = 85;
constexpr usize KEY_102ND                    = 86;
constexpr usize KEY_F11                      = 87;
constexpr usize KEY_F12                      = 88;
constexpr usize KEY_RO                       = 89;
constexpr usize KEY_KATAKANA                 = 90;
constexpr usize KEY_HIRAGANA                 = 91;
constexpr usize KEY_HENKAN                   = 92;
constexpr usize KEY_KATAKANAHIRAGANA         = 93;
constexpr usize KEY_MUHENKAN                 = 94;
constexpr usize KEY_KPJPCOMMA                = 95;
constexpr usize KEY_KPENTER                  = 96;
constexpr usize KEY_RIGHTCTRL                = 97;
constexpr usize KEY_KPSLASH                  = 98;
constexpr usize KEY_SYSRQ                    = 99;
constexpr usize KEY_RIGHTALT                 = 100;
constexpr usize KEY_LINEFEED                 = 101;
constexpr usize KEY_HOME                     = 102;
constexpr usize KEY_UP                       = 103;
constexpr usize KEY_PAGEUP                   = 104;
constexpr usize KEY_LEFT                     = 105;
constexpr usize KEY_RIGHT                    = 106;
constexpr usize KEY_END                      = 107;
constexpr usize KEY_DOWN                     = 108;
constexpr usize KEY_PAGEDOWN                 = 109;
constexpr usize KEY_INSERT                   = 110;
constexpr usize KEY_DELETE                   = 111;
constexpr usize KEY_MACRO                    = 112;
constexpr usize KEY_MUTE                     = 113;
constexpr usize KEY_VOLUMEDOWN               = 114;
constexpr usize KEY_VOLUMEUP                 = 115;
/* SC System Power Down */
constexpr usize KEY_POWER                    = 116;
constexpr usize KEY_KPEQUAL                  = 117;
constexpr usize KEY_KPPLUSMINUS              = 118;
constexpr usize KEY_PAUSE                    = 119;
/* AL Compiz Scale (Expose) */
constexpr usize KEY_SCALE                    = 120;

constexpr usize KEY_KPCOMMA                  = 121;
constexpr usize KEY_HANGEUL                  = 122;
constexpr usize KEY_HANGUEL                  = KEY_HANGEUL;
constexpr usize KEY_HANJA                    = 123;
constexpr usize KEY_YEN                      = 124;
constexpr usize KEY_LEFTMETA                 = 125;
constexpr usize KEY_RIGHTMETA                = 126;
constexpr usize KEY_COMPOSE                  = 127;

/* AC Stop */
constexpr usize KEY_STOP                     = 128;
constexpr usize KEY_AGAIN                    = 129;
/* AC Properties */
constexpr usize KEY_PROPS                    = 130;
/* AC Undo */
constexpr usize KEY_UNDO                     = 131;
constexpr usize KEY_FRONT                    = 132;
/* AC Copy */
constexpr usize KEY_COPY                     = 133;
/* AC Open */
constexpr usize KEY_OPEN                     = 134;
/* AC Paste */
constexpr usize KEY_PASTE                    = 135;
/* AC Search */
constexpr usize KEY_FIND                     = 136;
/* AC Cut */
constexpr usize KEY_CUT                      = 137;
/* AL Integrated Help Center */
constexpr usize KEY_HELP                     = 138;
/* Menu (show menu) */
constexpr usize KEY_MENU                     = 139;
/* AL Calculator */
constexpr usize KEY_CALC                     = 140;
constexpr usize KEY_SETUP                    = 141;
/* SC System Sleep */
constexpr usize KEY_SLEEP                    = 142;
/* System Wake Up */
constexpr usize KEY_WAKEUP                   = 143;
/* AL Local Machine Browser */
constexpr usize KEY_FILE                     = 144;
constexpr usize KEY_SENDFILE                 = 145;
constexpr usize KEY_DELETEFILE               = 146;
constexpr usize KEY_XFER                     = 147;
constexpr usize KEY_PROG1                    = 148;
constexpr usize KEY_PROG2                    = 149;
/* AL Internet Browser */
constexpr usize KEY_WWW                      = 150;
constexpr usize KEY_MSDOS                    = 151;
/* AL Terminal Lock/Screensaver */
constexpr usize KEY_COFFEE                   = 152;
constexpr usize KEY_SCREENLOCK               = KEY_COFFEE;
/* Display orientation for e.g. tablets */
constexpr usize KEY_ROTATE_DISPLAY           = 153;
constexpr usize KEY_DIRECTION                = KEY_ROTATE_DISPLAY;
constexpr usize KEY_CYCLEWINDOWS             = 154;
constexpr usize KEY_MAIL                     = 155;
/* AC Bookmarks */
constexpr usize KEY_BOOKMARKS                = 156;
constexpr usize KEY_COMPUTER                 = 157;
/* AC Back */
constexpr usize KEY_BACK                     = 158;
/* AC Forward */
constexpr usize KEY_FORWARD                  = 159;
constexpr usize KEY_CLOSECD                  = 160;
constexpr usize KEY_EJECTCD                  = 161;
constexpr usize KEY_EJECTCLOSECD             = 162;
constexpr usize KEY_NEXTSONG                 = 163;
constexpr usize KEY_PLAYPAUSE                = 164;
constexpr usize KEY_PREVIOUSSONG             = 165;
constexpr usize KEY_STOPCD                   = 166;
constexpr usize KEY_RECORD                   = 167;
constexpr usize KEY_REWIND                   = 168;
/* Media Select Telephone */
constexpr usize KEY_PHONE                    = 169;
constexpr usize KEY_ISO                      = 170;
/* AL Consumer Control Configuration */
constexpr usize KEY_CONFIG                   = 171;
/* AC Home */
constexpr usize KEY_HOMEPAGE                 = 172;
/* AC Refresh */
constexpr usize KEY_REFRESH                  = 173;
/* AC Exit */
constexpr usize KEY_EXIT                     = 174;
constexpr usize KEY_MOVE                     = 175;
constexpr usize KEY_EDIT                     = 176;
constexpr usize KEY_SCROLLUP                 = 177;
constexpr usize KEY_SCROLLDOWN               = 178;
constexpr usize KEY_KPLEFTPAREN              = 179;
constexpr usize KEY_KPRIGHTPAREN             = 180;
/* AC New */
constexpr usize KEY_NEW                      = 181;
/* AC Redo/Repeat */
constexpr usize KEY_REDO                     = 182;

constexpr usize KEY_F13                      = 183;
constexpr usize KEY_F14                      = 184;
constexpr usize KEY_F15                      = 185;
constexpr usize KEY_F16                      = 186;
constexpr usize KEY_F17                      = 187;
constexpr usize KEY_F18                      = 188;
constexpr usize KEY_F19                      = 189;
constexpr usize KEY_F20                      = 190;
constexpr usize KEY_F21                      = 191;
constexpr usize KEY_F22                      = 192;
constexpr usize KEY_F23                      = 193;
constexpr usize KEY_F24                      = 194;

constexpr usize KEY_PLAYCD                   = 200;
constexpr usize KEY_PAUSECD                  = 201;
constexpr usize KEY_PROG3                    = 202;
constexpr usize KEY_PROG4                    = 203;
/* AC Desktop Show All Applications */
constexpr usize KEY_ALL_APPLICATIONS         = 204;
constexpr usize KEY_DASHBOARD                = KEY_ALL_APPLICATIONS;
constexpr usize KEY_SUSPEND                  = 205;
/* AC Close */
constexpr usize KEY_CLOSE                    = 206;
constexpr usize KEY_PLAY                     = 207;
constexpr usize KEY_FASTFORWARD              = 208;
constexpr usize KEY_BASSBOOST                = 209;
/* AC Print */
constexpr usize KEY_PRINT                    = 210;
constexpr usize KEY_HP                       = 211;
constexpr usize KEY_CAMERA                   = 212;
constexpr usize KEY_SOUND                    = 213;
constexpr usize KEY_QUESTION                 = 214;
constexpr usize KEY_EMAIL                    = 215;
constexpr usize KEY_CHAT                     = 216;
constexpr usize KEY_SEARCH                   = 217;
constexpr usize KEY_CONNECT                  = 218;
/* AL Checkbook/Finance */
constexpr usize KEY_FINANCE                  = 219;
constexpr usize KEY_SPORT                    = 220;
constexpr usize KEY_SHOP                     = 221;
constexpr usize KEY_ALTERASE                 = 222;
/* AC Cancel */
constexpr usize KEY_CANCEL                   = 223;
constexpr usize KEY_BRIGHTNESSDOWN           = 224;
constexpr usize KEY_BRIGHTNESSUP             = 225;
constexpr usize KEY_MEDIA                    = 226;
/* Cycle between available video                                       \
       outputs (Monitor/LCD/TV-out/etc) */
constexpr usize KEY_SWITCHVIDEOMODE          = 227;
constexpr usize KEY_KBDILLUMTOGGLE           = 228;
constexpr usize KEY_KBDILLUMDOWN             = 229;
constexpr usize KEY_KBDILLUMUP               = 230;

/* AC Send */
constexpr usize KEY_SEND                     = 231;
/* AC Reply */
constexpr usize KEY_REPLY                    = 232;
/* AC Forward Msg */
constexpr usize KEY_FORWARDMAIL              = 233;
/* AC Save */
constexpr usize KEY_SAVE                     = 234;
constexpr usize KEY_DOCUMENTS                = 235;

constexpr usize KEY_BATTERY                  = 236;

constexpr usize KEY_BLUETOOTH                = 237;
constexpr usize KEY_WLAN                     = 238;
constexpr usize KEY_UWB                      = 239;

constexpr usize KEY_UNKNOWN                  = 240;

/* drive next video source */
constexpr usize KEY_VIDEO_NEXT               = 241;
/* drive previous video source */
constexpr usize KEY_VIDEO_PREV               = 242;
/* brightness up, after max is min */
constexpr usize KEY_BRIGHTNESS_CYCLE         = 243;
/* Set Auto Brightness: manual                                         \
 *brightness control is off,                                           \
rely on ambient */
constexpr usize KEY_BRIGHTNESS_AUTO          = 244;

constexpr usize KEY_BRIGHTNESS_ZERO          = KEY_BRIGHTNESS_AUTO;
/* display device to off state */
constexpr usize KEY_DISPLAY_OFF              = 245;

/* Wireless WAN (LTE, UMTS, GSM, etc.) */
constexpr usize KEY_WWAN                     = 246;
constexpr usize KEY_WIMAX                    = KEY_WWAN;
/* Key that controls all radios */
constexpr usize KEY_RFKILL                   = 247;

/* Mute / unmute the microphone */
constexpr usize KEY_MICMUTE                  = 248;

/* Code 255 is reserved for special needs of AT keyboard driver */
constexpr usize BTN_MISC                     = 0x100;
constexpr usize BTN_0                        = 0x100;
constexpr usize BTN_1                        = 0x101;
constexpr usize BTN_2                        = 0x102;
constexpr usize BTN_3                        = 0x103;
constexpr usize BTN_4                        = 0x104;
constexpr usize BTN_5                        = 0x105;
constexpr usize BTN_6                        = 0x106;
constexpr usize BTN_7                        = 0x107;
constexpr usize BTN_8                        = 0x108;
constexpr usize BTN_9                        = 0x109;

constexpr usize BTN_MOUSE                    = 0x110;
constexpr usize BTN_LEFT                     = 0x110;
constexpr usize BTN_RIGHT                    = 0x111;
constexpr usize BTN_MIDDLE                   = 0x112;
constexpr usize BTN_SIDE                     = 0x113;
constexpr usize BTN_EXTRA                    = 0x114;
constexpr usize BTN_FORWARD                  = 0x115;
constexpr usize BTN_BACK                     = 0x116;
constexpr usize BTN_TASK                     = 0x117;

constexpr usize BTN_JOYSTICK                 = 0x120;
constexpr usize BTN_TRIGGER                  = 0x120;
constexpr usize BTN_THUMB                    = 0x121;
constexpr usize BTN_THUMB2                   = 0x122;
constexpr usize BTN_TOP                      = 0x123;
constexpr usize BTN_TOP2                     = 0x124;
constexpr usize BTN_PINKIE                   = 0x125;
constexpr usize BTN_BASE                     = 0x126;
constexpr usize BTN_BASE2                    = 0x127;
constexpr usize BTN_BASE3                    = 0x128;
constexpr usize BTN_BASE4                    = 0x129;
constexpr usize BTN_BASE5                    = 0x12a;
constexpr usize BTN_BASE6                    = 0x12b;
constexpr usize BTN_DEAD                     = 0x12f;

constexpr usize BTN_GAMEPAD                  = 0x130;
constexpr usize BTN_SOUTH                    = 0x130;
constexpr usize BTN_A                        = BTN_SOUTH;
constexpr usize BTN_EAST                     = 0x131;
constexpr usize BTN_B                        = BTN_EAST;
constexpr usize BTN_C                        = 0x132;
constexpr usize BTN_NORTH                    = 0x133;
constexpr usize BTN_X                        = BTN_NORTH;
constexpr usize BTN_WEST                     = 0x134;
constexpr usize BTN_Y                        = BTN_WEST;
constexpr usize BTN_Z                        = 0x135;
constexpr usize BTN_TL                       = 0x136;
constexpr usize BTN_TR                       = 0x137;
constexpr usize BTN_TL2                      = 0x138;
constexpr usize BTN_TR2                      = 0x139;
constexpr usize BTN_SELECT                   = 0x13a;
constexpr usize BTN_START                    = 0x13b;
constexpr usize BTN_MODE                     = 0x13c;
constexpr usize BTN_THUMBL                   = 0x13d;
constexpr usize BTN_THUMBR                   = 0x13e;

constexpr usize BTN_DIGI                     = 0x140;
constexpr usize BTN_TOOL_PEN                 = 0x140;
constexpr usize BTN_TOOL_RUBBER              = 0x141;
constexpr usize BTN_TOOL_BRUSH               = 0x142;
constexpr usize BTN_TOOL_PENCIL              = 0x143;
constexpr usize BTN_TOOL_AIRBRUSH            = 0x144;
constexpr usize BTN_TOOL_FINGER              = 0x145;
constexpr usize BTN_TOOL_MOUSE               = 0x146;
constexpr usize BTN_TOOL_LENS                = 0x147;
/* Five fingers on trackpad */
constexpr usize BTN_TOOL_QUINTTAP            = 0x148;
constexpr usize BTN_STYLUS3                  = 0x149;
constexpr usize BTN_TOUCH                    = 0x14a;
constexpr usize BTN_STYLUS                   = 0x14b;
constexpr usize BTN_STYLUS2                  = 0x14c;
constexpr usize BTN_TOOL_DOUBLETAP           = 0x14d;
constexpr usize BTN_TOOL_TRIPLETAP           = 0x14e;
/* Four fingers on trackpad */
constexpr usize BTN_TOOL_QUADTAP             = 0x14f;

constexpr usize BTN_WHEEL                    = 0x150;
constexpr usize BTN_GEAR_DOWN                = 0x150;
constexpr usize BTN_GEAR_UP                  = 0x151;

constexpr usize KEY_OK                       = 0x160;
constexpr usize KEY_SELECT                   = 0x161;
constexpr usize KEY_GOTO                     = 0x162;
constexpr usize KEY_CLEAR                    = 0x163;
constexpr usize KEY_POWER2                   = 0x164;
constexpr usize KEY_OPTION                   = 0x165;
/* AL OEM Features/Tips/Tutorial */
constexpr usize KEY_INFO                     = 0x166;
constexpr usize KEY_TIME                     = 0x167;
constexpr usize KEY_VENDOR                   = 0x168;
constexpr usize KEY_ARCHIVE                  = 0x169;
/* Media Select Program Guide */
constexpr usize KEY_PROGRAM                  = 0x16a;
constexpr usize KEY_CHANNEL                  = 0x16b;
constexpr usize KEY_FAVORITES                = 0x16c;
constexpr usize KEY_EPG                      = 0x16d;
/* Media Select Home */
constexpr usize KEY_PVR                      = 0x16e;
constexpr usize KEY_MHP                      = 0x16f;
constexpr usize KEY_LANGUAGE                 = 0x170;
constexpr usize KEY_TITLE                    = 0x171;
constexpr usize KEY_SUBTITLE                 = 0x172;
constexpr usize KEY_ANGLE                    = 0x173;
/* AC View Toggle */
constexpr usize KEY_FULL_SCREEN              = 0x174;
constexpr usize KEY_ZOOM                     = KEY_FULL_SCREEN;
constexpr usize KEY_MODE                     = 0x175;
constexpr usize KEY_KEYBOARD                 = 0x176;
/* HUTRR37: Aspect */
constexpr usize KEY_ASPECT_RATIO             = 0x177;
constexpr usize KEY_SCREEN                   = KEY_ASPECT_RATIO;
/* Media Select Computer */
constexpr usize KEY_PC                       = 0x178;
/* Media Select TV */
constexpr usize KEY_TV                       = 0x179;
/* Media Select Cable */
constexpr usize KEY_TV2                      = 0x17a;
/* Media Select VCR */
constexpr usize KEY_VCR                      = 0x17b;
/* VCR Plus */
constexpr usize KEY_VCR2                     = 0x17c;
/* Media Select Satellite */
constexpr usize KEY_SAT                      = 0x17d;
constexpr usize KEY_SAT2                     = 0x17e;
/* Media Select CD */
constexpr usize KEY_CD                       = 0x17f;
/* Media Select Tape */
constexpr usize KEY_TAPE                     = 0x180;
constexpr usize KEY_RADIO                    = 0x181;
/* Media Select Tuner */
constexpr usize KEY_TUNER                    = 0x182;
constexpr usize KEY_PLAYER                   = 0x183;
constexpr usize KEY_TEXT                     = 0x184;
/* Media Select DVD */
constexpr usize KEY_DVD                      = 0x185;
constexpr usize KEY_AUX                      = 0x186;
constexpr usize KEY_MP3                      = 0x187;
/* AL Audio Browser */
constexpr usize KEY_AUDIO                    = 0x188;
/* AL Movie Browser */
constexpr usize KEY_VIDEO                    = 0x189;
constexpr usize KEY_DIRECTORY                = 0x18a;
constexpr usize KEY_LIST                     = 0x18b;
/* Media Select Messages */
constexpr usize KEY_MEMO                     = 0x18c;
constexpr usize KEY_CALENDAR                 = 0x18d;
constexpr usize KEY_RED                      = 0x18e;
constexpr usize KEY_GREEN                    = 0x18f;
constexpr usize KEY_YELLOW                   = 0x190;
constexpr usize KEY_BLUE                     = 0x191;
/* Channel Increment */
constexpr usize KEY_CHANNELUP                = 0x192;
/* Channel Decrement */
constexpr usize KEY_CHANNELDOWN              = 0x193;
constexpr usize KEY_FIRST                    = 0x194;
/* Recall Last */
constexpr usize KEY_LAST                     = 0x195;
constexpr usize KEY_AB                       = 0x196;
constexpr usize KEY_NEXT                     = 0x197;
constexpr usize KEY_RESTART                  = 0x198;
constexpr usize KEY_SLOW                     = 0x199;
constexpr usize KEY_SHUFFLE                  = 0x19a;
constexpr usize KEY_BREAK                    = 0x19b;
constexpr usize KEY_PREVIOUS                 = 0x19c;
constexpr usize KEY_DIGITS                   = 0x19d;
constexpr usize KEY_TEEN                     = 0x19e;
constexpr usize KEY_TWEN                     = 0x19f;
/* Media Select Video Phone */
constexpr usize KEY_VIDEOPHONE               = 0x1a0;
/* Media Select Games */
constexpr usize KEY_GAMES                    = 0x1a1;
/* AC Zoom In */
constexpr usize KEY_ZOOMIN                   = 0x1a2;
/* AC Zoom Out */
constexpr usize KEY_ZOOMOUT                  = 0x1a3;
/* AC Zoom */
constexpr usize KEY_ZOOMRESET                = 0x1a4;
/* AL Word Processor */
constexpr usize KEY_WORDPROCESSOR            = 0x1a5;
/* AL Text Editor */
constexpr usize KEY_EDITOR                   = 0x1a6;
/* AL Spreadsheet */
constexpr usize KEY_SPREADSHEET              = 0x1a7;
/* AL Graphics Editor */
constexpr usize KEY_GRAPHICSEDITOR           = 0x1a8;
/* AL Presentation App */
constexpr usize KEY_PRESENTATION             = 0x1a9;
/* AL Database App */
constexpr usize KEY_DATABASE                 = 0x1aa;
/* AL Newsreader */
constexpr usize KEY_NEWS                     = 0x1ab;
/* AL Voicemail */
constexpr usize KEY_VOICEMAIL                = 0x1ac;
/* AL Contacts/Address Book */
constexpr usize KEY_ADDRESSBOOK              = 0x1ad;
/* AL Instant Messaging */
constexpr usize KEY_MESSENGER                = 0x1ae;
/* Turn display (LCD) on and off */
constexpr usize KEY_DISPLAYTOGGLE            = 0x1af;
constexpr usize KEY_BRIGHTNESS_TOGGLE        = KEY_DISPLAYTOGGLE;
/* AL Spell Check */
constexpr usize KEY_SPELLCHECK               = 0x1b0;
/* AL Logoff */
constexpr usize KEY_LOGOFF                   = 0x1b1;

constexpr usize KEY_DOLLAR                   = 0x1b2;
constexpr usize KEY_EURO                     = 0x1b3;

/* Consumer - transport controls */
constexpr usize KEY_FRAMEBACK                = 0x1b4;
constexpr usize KEY_FRAMEFORWARD             = 0x1b5;
/* GenDesc - system context menu */
constexpr usize KEY_CONTEXT_MENU             = 0x1b6;
/* Consumer - transport control */
constexpr usize KEY_MEDIA_REPEAT             = 0x1b7;
/* 10 channels up (10+) */
constexpr usize KEY_10CHANNELSUP             = 0x1b8;
/* 10 channels down (10-) */
constexpr usize KEY_10CHANNELSDOWN           = 0x1b9;
/* AL Image Browser */
constexpr usize KEY_IMAGES                   = 0x1ba;
/* Show/hide the notification center */
constexpr usize KEY_NOTIFICATION_CENTER      = 0x1bc;
/* Answer incoming call */
constexpr usize KEY_PICKUP_PHONE             = 0x1bd;
/* Decline incoming call */
constexpr usize KEY_HANGUP_PHONE             = 0x1be;
/* AL Phone Syncing */
constexpr usize KEY_LINK_PHONE               = 0x1bf;

constexpr usize KEY_DEL_EOL                  = 0x1c0;
constexpr usize KEY_DEL_EOS                  = 0x1c1;
constexpr usize KEY_INS_LINE                 = 0x1c2;
constexpr usize KEY_DEL_LINE                 = 0x1c3;

constexpr usize KEY_FN                       = 0x1d0;
constexpr usize KEY_FN_ESC                   = 0x1d1;
constexpr usize KEY_FN_F1                    = 0x1d2;
constexpr usize KEY_FN_F2                    = 0x1d3;
constexpr usize KEY_FN_F3                    = 0x1d4;
constexpr usize KEY_FN_F4                    = 0x1d5;
constexpr usize KEY_FN_F5                    = 0x1d6;
constexpr usize KEY_FN_F6                    = 0x1d7;
constexpr usize KEY_FN_F7                    = 0x1d8;
constexpr usize KEY_FN_F8                    = 0x1d9;
constexpr usize KEY_FN_F9                    = 0x1da;
constexpr usize KEY_FN_F10                   = 0x1db;
constexpr usize KEY_FN_F11                   = 0x1dc;
constexpr usize KEY_FN_F12                   = 0x1dd;
constexpr usize KEY_FN_1                     = 0x1de;
constexpr usize KEY_FN_2                     = 0x1df;
constexpr usize KEY_FN_D                     = 0x1e0;
constexpr usize KEY_FN_E                     = 0x1e1;
constexpr usize KEY_FN_F                     = 0x1e2;
constexpr usize KEY_FN_S                     = 0x1e3;
constexpr usize KEY_FN_B                     = 0x1e4;
constexpr usize KEY_FN_RIGHT_SHIFT           = 0x1e5;

constexpr usize KEY_BRL_DOT1                 = 0x1f1;
constexpr usize KEY_BRL_DOT2                 = 0x1f2;
constexpr usize KEY_BRL_DOT3                 = 0x1f3;
constexpr usize KEY_BRL_DOT4                 = 0x1f4;
constexpr usize KEY_BRL_DOT5                 = 0x1f5;
constexpr usize KEY_BRL_DOT6                 = 0x1f6;
constexpr usize KEY_BRL_DOT7                 = 0x1f7;
constexpr usize KEY_BRL_DOT8                 = 0x1f8;
constexpr usize KEY_BRL_DOT9                 = 0x1f9;
constexpr usize KEY_BRL_DOT10                = 0x1fa;

/* used by phones, remote controls, */
constexpr usize KEY_NUMERIC_0                = 0x200;
/* and other keypads */
constexpr usize KEY_NUMERIC_1                = 0x201;
constexpr usize KEY_NUMERIC_2                = 0x202;
constexpr usize KEY_NUMERIC_3                = 0x203;
constexpr usize KEY_NUMERIC_4                = 0x204;
constexpr usize KEY_NUMERIC_5                = 0x205;
constexpr usize KEY_NUMERIC_6                = 0x206;
constexpr usize KEY_NUMERIC_7                = 0x207;
constexpr usize KEY_NUMERIC_8                = 0x208;
constexpr usize KEY_NUMERIC_9                = 0x209;
constexpr usize KEY_NUMERIC_STAR             = 0x20a;
constexpr usize KEY_NUMERIC_POUND            = 0x20b;
/* Phone key A - HUT Telephony 0xb9 */
constexpr usize KEY_NUMERIC_A                = 0x20c;
constexpr usize KEY_NUMERIC_B                = 0x20d;
constexpr usize KEY_NUMERIC_C                = 0x20e;
constexpr usize KEY_NUMERIC_D                = 0x20f;

constexpr usize KEY_CAMERA_FOCUS             = 0x210;
/* WiFi Protected Setup key */
constexpr usize KEY_WPS_BUTTON               = 0x211;

/* Request switch touchpad on or off */
constexpr usize KEY_TOUCHPAD_TOGGLE          = 0x212;
constexpr usize KEY_TOUCHPAD_ON              = 0x213;
constexpr usize KEY_TOUCHPAD_OFF             = 0x214;

constexpr usize KEY_CAMERA_ZOOMIN            = 0x215;
constexpr usize KEY_CAMERA_ZOOMOUT           = 0x216;
constexpr usize KEY_CAMERA_UP                = 0x217;
constexpr usize KEY_CAMERA_DOWN              = 0x218;
constexpr usize KEY_CAMERA_LEFT              = 0x219;
constexpr usize KEY_CAMERA_RIGHT             = 0x21a;

constexpr usize KEY_ATTENDANT_ON             = 0x21b;
constexpr usize KEY_ATTENDANT_OFF            = 0x21c;
/* Attendant call on or off */
constexpr usize KEY_ATTENDANT_TOGGLE         = 0x21d;
/* Reading light on or off */
constexpr usize KEY_LIGHTS_TOGGLE            = 0x21e;

constexpr usize BTN_DPAD_UP                  = 0x220;
constexpr usize BTN_DPAD_DOWN                = 0x221;
constexpr usize BTN_DPAD_LEFT                = 0x222;
constexpr usize BTN_DPAD_RIGHT               = 0x223;

/* Ambient light sensor */
constexpr usize KEY_ALS_TOGGLE               = 0x230;
/* Display rotation lock */
constexpr usize KEY_ROTATE_LOCK_TOGGLE       = 0x231;
/* Display refresh rate toggle */
constexpr usize KEY_REFRESH_RATE_TOGGLE      = 0x232;

/* AL Button Configuration */
constexpr usize KEY_BUTTONCONFIG             = 0x240;
/* AL Task/Project Manager */
constexpr usize KEY_TASKMANAGER              = 0x241;
/* AL Log/Journal/Timecard */
constexpr usize KEY_JOURNAL                  = 0x242;
/* AL Control Panel */
constexpr usize KEY_CONTROLPANEL             = 0x243;
/* AL Select Task/Application */
constexpr usize KEY_APPSELECT                = 0x244;
/* AL Screen Saver */
constexpr usize KEY_SCREENSAVER              = 0x245;
/* Listening Voice Command */
constexpr usize KEY_VOICECOMMAND             = 0x246;
/* AL Context-aware desktop assistant */
constexpr usize KEY_ASSISTANT                = 0x247;
/* AC Next Keyboard Layout Select */
constexpr usize KEY_KBD_LAYOUT_NEXT          = 0x248;
/* Show/hide emoji picker (HUTRR101) */
constexpr usize KEY_EMOJI_PICKER             = 0x249;
/* Start or Stop Voice Dictation Session (HUTRR99) \
 */
constexpr usize KEY_DICTATE                  = 0x24a;
/* Enables programmatic access to camera devices. (HUTRR72) */
constexpr usize KEY_CAMERA_ACCESS_ENABLE     = 0x24b;
/* Disables programmatic access to camera devices. (HUTRR72) */
constexpr usize KEY_CAMERA_ACCESS_DISABLE    = 0x24c;
/* Toggles the current state of the camera access control.
 * (HUTRR72) \
 */
constexpr usize KEY_CAMERA_ACCESS_TOGGLE     = 0x24d;
/* Toggles the system bound accessibility UI/command (HUTRR116) */
constexpr usize KEY_ACCESSIBILITY            = 0x24e;
/* Toggles the system-wide "Do Not Disturb" control (HUTRR94)*/
constexpr usize KEY_DO_NOT_DISTURB           = 0x24f;

/* Set Brightness to Minimum */
constexpr usize KEY_BRIGHTNESS_MIN           = 0x250;
/* Set Brightness to Maximum */
constexpr usize KEY_BRIGHTNESS_MAX           = 0x251;

constexpr usize KEY_KBDINPUTASSIST_PREV      = 0x260;
constexpr usize KEY_KBDINPUTASSIST_NEXT      = 0x261;
constexpr usize KEY_KBDINPUTASSIST_PREVGROUP = 0x262;
constexpr usize KEY_KBDINPUTASSIST_NEXTGROUP = 0x263;
constexpr usize KEY_KBDINPUTASSIST_ACCEPT    = 0x264;
constexpr usize KEY_KBDINPUTASSIST_CANCEL    = 0x265;

/* Diagonal movement keys */
constexpr usize KEY_RIGHT_UP                 = 0x266;
constexpr usize KEY_RIGHT_DOWN               = 0x267;
constexpr usize KEY_LEFT_UP                  = 0x268;
constexpr usize KEY_LEFT_DOWN                = 0x269;

/* Show Device's Root Menu */
constexpr usize KEY_ROOT_MENU                = 0x26a;
/* Show Top Menu of the Media (e.g. DVD) */
constexpr usize KEY_MEDIA_TOP_MENU           = 0x26b;
constexpr usize KEY_NUMERIC_11               = 0x26c;
constexpr usize KEY_NUMERIC_12               = 0x26d;
/*
 * Toggle Audio Description: refers to an audio service that helps blind and
 * visually impaired consumers understand the action in a program. Note: in
 * some countries this is referred to as "Video Description".
 */
constexpr usize KEY_AUDIO_DESC               = 0x26e;
constexpr usize KEY_3D_MODE                  = 0x26f;
constexpr usize KEY_NEXT_FAVORITE            = 0x270;
constexpr usize KEY_STOP_RECORD              = 0x271;
constexpr usize KEY_PAUSE_RECORD             = 0x272;
/* Video on Demand */
constexpr usize KEY_VOD                      = 0x273;
constexpr usize KEY_UNMUTE                   = 0x274;
constexpr usize KEY_FASTREVERSE              = 0x275;
constexpr usize KEY_SLOWREVERSE              = 0x276;
/*
 * Control a data application associated with the currently viewed channel,
 * e.g. teletext or data broadcast application (MHEG, MHP, HbbTV, etc.)
 */
constexpr usize KEY_DATA                     = 0x277;
constexpr usize KEY_ONSCREEN_KEYBOARD        = 0x278;
/* Electronic privacy screen control */
constexpr usize KEY_PRIVACY_SCREEN_TOGGLE    = 0x279;

/* Select an area of screen to be copied */
constexpr usize KEY_SELECTIVE_SCREENSHOT     = 0x27a;

/* Move the focus to the next or previous user controllable element within a UI
 * container */
constexpr usize KEY_NEXT_ELEMENT             = 0x27b;
constexpr usize KEY_PREVIOUS_ELEMENT         = 0x27c;

/* Toggle Autopilot engagement */
constexpr usize KEY_AUTOPILOT_ENGAGE_TOGGLE  = 0x27d;

/* Shortcut Keys */
constexpr usize KEY_MARK_WAYPOINT            = 0x27e;
constexpr usize KEY_SOS                      = 0x27f;
constexpr usize KEY_NAV_CHART                = 0x280;
constexpr usize KEY_FISHING_CHART            = 0x281;
constexpr usize KEY_SINGLE_RANGE_RADAR       = 0x282;
constexpr usize KEY_DUAL_RANGE_RADAR         = 0x283;
constexpr usize KEY_RADAR_OVERLAY            = 0x284;
constexpr usize KEY_TRADITIONAL_SONAR        = 0x285;
constexpr usize KEY_CLEARVU_SONAR            = 0x286;
constexpr usize KEY_SIDEVU_SONAR             = 0x287;
constexpr usize KEY_NAV_INFO                 = 0x288;
constexpr usize KEY_BRIGHTNESS_MENU          = 0x289;

/*
 * Some keyboards have keys which do not have a defined meaning, these keys
 * are intended to be programmed / bound to macros by the user. For most
 * keyboards with these macro-keys the key-sequence to inject, or action to
 * take, is all handled by software on the host side. So from the kernel's
 * point of view these are just normal keys.
 *
 * The KEY_MACRO# codes below are intended for such keys, which may be labeled
 * e.g. G1-G18, or S1 - S30. The KEY_MACRO# codes MUST NOT be used for keys
 * where the marking on the key does indicate a defined meaning / purpose.
 *
 * The KEY_MACRO# codes MUST also NOT be used as fallback for when no existing
 * KEY_FOO define matches the marking / purpose. In this case a new KEY_FOO
 * define MUST be added.
 */
constexpr usize KEY_MACRO1                   = 0x290;
constexpr usize KEY_MACRO2                   = 0x291;
constexpr usize KEY_MACRO3                   = 0x292;
constexpr usize KEY_MACRO4                   = 0x293;
constexpr usize KEY_MACRO5                   = 0x294;
constexpr usize KEY_MACRO6                   = 0x295;
constexpr usize KEY_MACRO7                   = 0x296;
constexpr usize KEY_MACRO8                   = 0x297;
constexpr usize KEY_MACRO9                   = 0x298;
constexpr usize KEY_MACRO10                  = 0x299;
constexpr usize KEY_MACRO11                  = 0x29a;
constexpr usize KEY_MACRO12                  = 0x29b;
constexpr usize KEY_MACRO13                  = 0x29c;
constexpr usize KEY_MACRO14                  = 0x29d;
constexpr usize KEY_MACRO15                  = 0x29e;
constexpr usize KEY_MACRO16                  = 0x29f;
constexpr usize KEY_MACRO17                  = 0x2a0;
constexpr usize KEY_MACRO18                  = 0x2a1;
constexpr usize KEY_MACRO19                  = 0x2a2;
constexpr usize KEY_MACRO20                  = 0x2a3;
constexpr usize KEY_MACRO21                  = 0x2a4;
constexpr usize KEY_MACRO22                  = 0x2a5;
constexpr usize KEY_MACRO23                  = 0x2a6;
constexpr usize KEY_MACRO24                  = 0x2a7;
constexpr usize KEY_MACRO25                  = 0x2a8;
constexpr usize KEY_MACRO26                  = 0x2a9;
constexpr usize KEY_MACRO27                  = 0x2aa;
constexpr usize KEY_MACRO28                  = 0x2ab;
constexpr usize KEY_MACRO29                  = 0x2ac;
constexpr usize KEY_MACRO30                  = 0x2ad;

/*
 * Some keyboards with the macro-keys described above have some extra keys
 * for controlling the host-side software responsible for the macro handling:
 * -A macro recording start/stop key. Note that not all keyboards which emit
 *  KEY_MACRO_RECORD_START will also emit KEY_MACRO_RECORD_STOP if
 *  KEY_MACRO_RECORD_STOP is not advertised, then KEY_MACRO_RECORD_START
 *  should be interpreted as a recording start/stop toggle;
 * -Keys for switching between different macro (pre)sets, either a key for
 *  cycling through the configured presets or keys to directly select a preset.
 */
constexpr usize KEY_MACRO_RECORD_START       = 0x2b0;
constexpr usize KEY_MACRO_RECORD_STOP        = 0x2b1;
constexpr usize KEY_MACRO_PRESET_CYCLE       = 0x2b2;
constexpr usize KEY_MACRO_PRESET1            = 0x2b3;
constexpr usize KEY_MACRO_PRESET2            = 0x2b4;
constexpr usize KEY_MACRO_PRESET3            = 0x2b5;

/*
 * Some keyboards have a buildin LCD panel where the contents are controlled
 * by the host. Often these have a number of keys directly below the LCD
 * intended for controlling a menu shown on the LCD. These keys often don't
 * have any labeling so we just name them KEY_KBD_LCD_MENU#
 */
constexpr usize KEY_KBD_LCD_MENU1            = 0x2b8;
constexpr usize KEY_KBD_LCD_MENU2            = 0x2b9;
constexpr usize KEY_KBD_LCD_MENU3            = 0x2ba;
constexpr usize KEY_KBD_LCD_MENU4            = 0x2bb;
constexpr usize KEY_KBD_LCD_MENU5            = 0x2bc;

constexpr usize BTN_TRIGGER_HAPPY            = 0x2c0;
constexpr usize BTN_TRIGGER_HAPPY1           = 0x2c0;
constexpr usize BTN_TRIGGER_HAPPY2           = 0x2c1;
constexpr usize BTN_TRIGGER_HAPPY3           = 0x2c2;
constexpr usize BTN_TRIGGER_HAPPY4           = 0x2c3;
constexpr usize BTN_TRIGGER_HAPPY5           = 0x2c4;
constexpr usize BTN_TRIGGER_HAPPY6           = 0x2c5;
constexpr usize BTN_TRIGGER_HAPPY7           = 0x2c6;
constexpr usize BTN_TRIGGER_HAPPY8           = 0x2c7;
constexpr usize BTN_TRIGGER_HAPPY9           = 0x2c8;
constexpr usize BTN_TRIGGER_HAPPY10          = 0x2c9;
constexpr usize BTN_TRIGGER_HAPPY11          = 0x2ca;
constexpr usize BTN_TRIGGER_HAPPY12          = 0x2cb;
constexpr usize BTN_TRIGGER_HAPPY13          = 0x2cc;
constexpr usize BTN_TRIGGER_HAPPY14          = 0x2cd;
constexpr usize BTN_TRIGGER_HAPPY15          = 0x2ce;
constexpr usize BTN_TRIGGER_HAPPY16          = 0x2cf;
constexpr usize BTN_TRIGGER_HAPPY17          = 0x2d0;
constexpr usize BTN_TRIGGER_HAPPY18          = 0x2d1;
constexpr usize BTN_TRIGGER_HAPPY19          = 0x2d2;
constexpr usize BTN_TRIGGER_HAPPY20          = 0x2d3;
constexpr usize BTN_TRIGGER_HAPPY21          = 0x2d4;
constexpr usize BTN_TRIGGER_HAPPY22          = 0x2d5;
constexpr usize BTN_TRIGGER_HAPPY23          = 0x2d6;
constexpr usize BTN_TRIGGER_HAPPY24          = 0x2d7;
constexpr usize BTN_TRIGGER_HAPPY25          = 0x2d8;
constexpr usize BTN_TRIGGER_HAPPY26          = 0x2d9;
constexpr usize BTN_TRIGGER_HAPPY27          = 0x2da;
constexpr usize BTN_TRIGGER_HAPPY28          = 0x2db;
constexpr usize BTN_TRIGGER_HAPPY29          = 0x2dc;
constexpr usize BTN_TRIGGER_HAPPY30          = 0x2dd;
constexpr usize BTN_TRIGGER_HAPPY31          = 0x2de;
constexpr usize BTN_TRIGGER_HAPPY32          = 0x2df;
constexpr usize BTN_TRIGGER_HAPPY33          = 0x2e0;
constexpr usize BTN_TRIGGER_HAPPY34          = 0x2e1;
constexpr usize BTN_TRIGGER_HAPPY35          = 0x2e2;
constexpr usize BTN_TRIGGER_HAPPY36          = 0x2e3;
constexpr usize BTN_TRIGGER_HAPPY37          = 0x2e4;
constexpr usize BTN_TRIGGER_HAPPY38          = 0x2e5;
constexpr usize BTN_TRIGGER_HAPPY39          = 0x2e6;
constexpr usize BTN_TRIGGER_HAPPY40          = 0x2e7;

/* We avoid low common keys in module aliases so they don't get huge. */
constexpr usize KEY_MIN_INTERESTING          = KEY_MUTE;
constexpr usize KEY_MAX                      = 0x2ff;
constexpr usize KEY_CNT                      = (KEY_MAX + 1);

/*
 * Relative axes
 */
constexpr usize REL_X                        = 0x00;
constexpr usize REL_Y                        = 0x01;
constexpr usize REL_Z                        = 0x02;
constexpr usize REL_RX                       = 0x03;
constexpr usize REL_RY                       = 0x04;
constexpr usize REL_RZ                       = 0x05;
constexpr usize REL_HWHEEL                   = 0x06;
constexpr usize REL_DIAL                     = 0x07;
constexpr usize REL_WHEEL                    = 0x08;
constexpr usize REL_MISC                     = 0x09;
/*
 * 0x0a is reserved and should not be used in input drivers.
 * It was used by HID as REL_MISC+1 and userspace needs to detect if
 * the next REL_* event is correct or is just REL_MISC + n.
 * We define here REL_RESERVED so userspace can rely on it and detect
 * the situation described above.
 */
constexpr usize REL_RESERVED                 = 0x0a;
constexpr usize REL_WHEEL_HI_RES             = 0x0b;
constexpr usize REL_HWHEEL_HI_RES            = 0x0c;
constexpr usize REL_MAX                      = 0x0f;
constexpr usize REL_CNT                      = (REL_MAX + 1);

/*
 * Absolute axes
 */
constexpr usize ABS_X                        = 0x00;
constexpr usize ABS_Y                        = 0x01;
constexpr usize ABS_Z                        = 0x02;
constexpr usize ABS_RX                       = 0x03;
constexpr usize ABS_RY                       = 0x04;
constexpr usize ABS_RZ                       = 0x05;
constexpr usize ABS_THROTTLE                 = 0x06;
constexpr usize ABS_RUDDER                   = 0x07;
constexpr usize ABS_WHEEL                    = 0x08;
constexpr usize ABS_GAS                      = 0x09;
constexpr usize ABS_BRAKE                    = 0x0a;
constexpr usize ABS_HAT0X                    = 0x10;
constexpr usize ABS_HAT0Y                    = 0x11;
constexpr usize ABS_HAT1X                    = 0x12;
constexpr usize ABS_HAT1Y                    = 0x13;
constexpr usize ABS_HAT2X                    = 0x14;
constexpr usize ABS_HAT2Y                    = 0x15;
constexpr usize ABS_HAT3X                    = 0x16;
constexpr usize ABS_HAT3Y                    = 0x17;
constexpr usize ABS_PRESSURE                 = 0x18;
constexpr usize ABS_DISTANCE                 = 0x19;
constexpr usize ABS_TILT_X                   = 0x1a;
constexpr usize ABS_TILT_Y                   = 0x1b;
constexpr usize ABS_TOOL_WIDTH               = 0x1c;

constexpr usize ABS_VOLUME                   = 0x20;
constexpr usize ABS_PROFILE                  = 0x21;

constexpr usize ABS_MISC                     = 0x28;

/*
 * 0x2e is reserved and should not be used in input drivers.
 * It was used by HID as ABS_MISC+6 and userspace needs to detect if
 * the next ABS_* event is correct or is just ABS_MISC + n.
 * We define here ABS_RESERVED so userspace can rely on it and detect
 * the situation described above.
 */
constexpr usize ABS_RESERVED                 = 0x2e;

/* MT slot being modified */
constexpr usize ABS_MT_SLOT                  = 0x2f;
/* Major axis of touching ellipse */
constexpr usize ABS_MT_TOUCH_MAJOR           = 0x30;
/* Minor axis (omit if circular) */
constexpr usize ABS_MT_TOUCH_MINOR           = 0x31;
/* Major axis of approaching ellipse */
constexpr usize ABS_MT_WIDTH_MAJOR           = 0x32;
/* Minor axis (omit if circular) */
constexpr usize ABS_MT_WIDTH_MINOR           = 0x33;
/* Ellipse orientation */
constexpr usize ABS_MT_ORIENTATION           = 0x34;
/* Center X touch position */
constexpr usize ABS_MT_POSITION_X            = 0x35;
/* Center Y touch position */
constexpr usize ABS_MT_POSITION_Y            = 0x36;
/* Type of touching device */
constexpr usize ABS_MT_TOOL_TYPE             = 0x37;
/* Group a set of packets as a blob */
constexpr usize ABS_MT_BLOB_ID               = 0x38;
/* Unique ID of initiated contact */
constexpr usize ABS_MT_TRACKING_ID           = 0x39;
/* Pressure on contact area */
constexpr usize ABS_MT_PRESSURE              = 0x3a;
/* Contact hover distance */
constexpr usize ABS_MT_DISTANCE              = 0x3b;
/* Center X tool position */
constexpr usize ABS_MT_TOOL_X                = 0x3c;
/* Center Y tool position */
constexpr usize ABS_MT_TOOL_Y                = 0x3d;

constexpr usize ABS_MAX                      = 0x3f;
constexpr usize ABS_CNT                      = (ABS_MAX + 1);

/*
 * Switch events
 */
/* set = lid shut */
constexpr usize SW_LID                       = 0x00;
/* set = tablet mode */
constexpr usize SW_TABLET_MODE               = 0x01;
/* set = inserted */
constexpr usize SW_HEADPHONE_INSERT          = 0x02;
/* rfkill master switch, type "any"                                   \
        set = radio enabled */
constexpr usize SW_RFKILL_ALL                = 0x03;
/* deprecated */
constexpr usize SW_RADIO                     = SW_RFKILL_ALL;
/* set = inserted */
constexpr usize SW_MICROPHONE_INSERT         = 0x04;
/* set = plugged into dock */
constexpr usize SW_DOCK                      = 0x05;
/* set = inserted */
constexpr usize SW_LINEOUT_INSERT            = 0x06;
/* set = mechanical switch set */
constexpr usize SW_JACK_PHYSICAL_INSERT      = 0x07;
/* set = inserted */
constexpr usize SW_VIDEOOUT_INSERT           = 0x08;
/* set = lens covered */
constexpr usize SW_CAMERA_LENS_COVER         = 0x09;
/* set = keypad slide out */
constexpr usize SW_KEYPAD_SLIDE              = 0x0a;
/* set = front proximity sensor active */
constexpr usize SW_FRONT_PROXIMITY           = 0x0b;
/* set = rotate locked/disabled */
constexpr usize SW_ROTATE_LOCK               = 0x0c;
/* set = inserted */
constexpr usize SW_LINEIN_INSERT             = 0x0d;
/* set = device disabled */
constexpr usize SW_MUTE_DEVICE               = 0x0e;
/* set = pen inserted */
constexpr usize SW_PEN_INSERTED              = 0x0f;
/* set = cover closed */
constexpr usize SW_MACHINE_COVER             = 0x10;
/* set = USB audio device connected */
constexpr usize SW_USB_INSERT                = 0x11;
constexpr usize SW_MAX                       = 0x11;
constexpr usize SW_CNT                       = (SW_MAX + 1);

/*
 * Misc events
 */
constexpr usize MSC_SERIAL                   = 0x00;
constexpr usize MSC_PULSELED                 = 0x01;
constexpr usize MSC_GESTURE                  = 0x02;
constexpr usize MSC_RAW                      = 0x03;
constexpr usize MSC_SCAN                     = 0x04;
constexpr usize MSC_TIMESTAMP                = 0x05;
constexpr usize MSC_MAX                      = 0x07;
constexpr usize MSC_CNT                      = (MSC_MAX + 1);

/*
 * LEDs
 */
constexpr usize LED_NUML                     = 0x00;
constexpr usize LED_CAPSL                    = 0x01;
constexpr usize LED_SCROLLL                  = 0x02;
constexpr usize LED_COMPOSE                  = 0x03;
constexpr usize LED_KANA                     = 0x04;
constexpr usize LED_SLEEP                    = 0x05;
constexpr usize LED_SUSPEND                  = 0x06;
constexpr usize LED_MUTE                     = 0x07;
constexpr usize LED_MISC                     = 0x08;
constexpr usize LED_MAIL                     = 0x09;
constexpr usize LED_CHARGING                 = 0x0a;
constexpr usize LED_MAX                      = 0x0f;
constexpr usize LED_CNT                      = (LED_MAX + 1);

/*
 * Autorepeat values
 */
constexpr usize REP_DELAY                    = 0x00;
constexpr usize REP_PERIOD                   = 0x01;
constexpr usize REP_MAX                      = 0x01;
constexpr usize REP_CNT                      = (REP_MAX + 1);

/*
 * Sounds
 */
constexpr usize SND_CLICK                    = 0x00;
constexpr usize SND_BELL                     = 0x01;
constexpr usize SND_TONE                     = 0x02;
constexpr usize SND_MAX                      = 0x07;
constexpr usize SND_CNT                      = (SND_MAX + 1);
