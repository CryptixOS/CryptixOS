
/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/Drivers/PCSpeaker.hpp>
    #include <Arch/x86_64/IO.hpp>
#endif
#include <Boot/BootInfo.hpp>

#include <Drivers/Serial.hpp>
#include <Drivers/Terminal.hpp>
#include <Drivers/VideoTerminal.hpp>

#include <Memory/PMM.hpp>

Vector<Terminal*> Terminal::s_Terminals = {};
Terminal*         s_ActiveTerminal      = nullptr;

Terminal::Terminal()
{
    if (!s_ActiveTerminal) s_ActiveTerminal = this;
}

void Terminal::Resize(const winsize& windowSize) {}

void Terminal::PutChar(u64 c)
{
    PutCharImpl(c);

    Flush();
}
void Terminal::PutCharImpl(u64 c)
{
    if (!m_Initialized) return;

    ScopedLock guard(m_Lock);
    if (c == 0x18 || c == 0x1a)
    {
        m_State = State::eNormal;
        return;
    }

    if (m_State == State::eEscape)
    {
        OnEscapeChar(c);
        return;
    }

    auto [x, y] = GetCursorPos();
    switch (c)
    {
        case '\0': return;
        case '\a': Bell(); return;
        case '\b':
            SetCursorPos(x - 1, y);
            RawPutChar(' ');

            SetCursorPos(x - 1, y);
            return;
        case '\t':
            if ((x / m_TabSize + 1) >= m_Size.ws_col)
            {
                SetCursorPos(m_Size.ws_col - 1, y);
                return;
            }
            SetCursorPos((x / m_TabSize + 1) * m_TabSize, y);
            return;
        case '\n':
        case 0x0b:
        case 0x0c:
            if (y == m_ScrollBottomMargin - 1)
            {
                ScrollDown();
                SetCursorPos(0, y);
            }
            else SetCursorPos(0, ++y);
            return;
        case '\r': SetCursorPos(0, y); return;
        case 14: return;
        case 15: return;
        case '\e': /*m_State = State::eEscape;*/ return;
        case 0x1a: m_State = State::eNormal; return;
        case 0x7f: return;
    }

    // TODO(v1tr10l7): charset #1

    if (c >= 0x20 && c <= 0x7e) return RawPutChar(c);
    RawPutChar(0xfe);
}

void Terminal::PrintString(std::string_view str)
{
    if (!m_Initialized) return;

    for (auto c : str) PutCharImpl(c);
}

void Terminal::Bell()
{
#ifdef CTOS_TARGET_X86_64
    PCSpeaker::ToneOn(1000);
    IO::Delay(1000);
    PCSpeaker::ToneOff();
#endif
}

Terminal*                Terminal::GetPrimary() { return s_ActiveTerminal; }
const Vector<Terminal*>& Terminal::EnumerateTerminals()
{
    static bool initialized = false;
    if (initialized) return s_Terminals;

    usize         framebufferCount = 0;
    Framebuffer** framebuffers = BootInfo::GetFramebuffers(framebufferCount);

    s_ActiveTerminal           = new VideoTerminal(*framebuffers[0]);
    s_Terminals.PushBack(s_ActiveTerminal);

    LogInfo("Terminal: Initialized {} terminals", s_Terminals.Size());
    initialized = true;
    return s_Terminals;
}

void Terminal::OnEscapeChar(char c)
{
    auto [x, y] = GetCursorPos();

    switch (c)
    {
        case '[':
            for (usize i = 0; i < MAX_ESC_PARAMETER_COUNT; i++)
                m_EscapeValues[i] = 0;
            m_EscapeValueCount = 0;
            return;
        case ']': break;
        case '_': break;
        // Reset
        case 'c':
            Reset();
            Clear();
            break;
        // Line Feed
        case 'D':
            if (y == m_ScrollBottomMargin - 1)
            {
                ScrollDown();
                SetCursorPos(x, y);
                ++y;
            }
            SetCursorPos(x, y);
            break;
        // New Line
        case 'E':
            if (y == m_ScrollBottomMargin - 1)
            {
                ScrollDown();
                ++y;
            }
            SetCursorPos(0, y);
            break;
        // Set tab stop at current column
        case 'H': break;
        // Reverse Linefeed
        case 'M':
            if (y == m_ScrollTopMargin)
            {
                ScrollUp();
                SetCursorPos(0, y);
                --y;
            }
            SetCursorPos(0, y);
            break;
        // TODO(v1tr10l7): DEC private identification
        case 'Z': break;
        // Save current state
        case '7': SaveState(); break;
        // Restore saved state
        case '8': RestoreState(); break;
        // Start sequence selecting character set
        case '%': break;
        // Start sequence defining G0 character set
        case '(':
        // Start sequence defining G1 character set
        case ')': break;
    }

    m_State = State::eNormal;
}

void Terminal::Reset()
{
    m_TabSize            = 4;
    m_EscapeValueCount   = 0;
    m_ScrollTopMargin    = 0;
    m_ScrollBottomMargin = m_Size.ws_row;
}
bool Terminal::DecSpecialPrint(u8 c)
{
    switch (c)
    {
        case '`': RawPutChar(0x04); return true;
        case '0': RawPutChar(0xdb); return true;
        case '-': RawPutChar(0x18); return true;
        case ',': RawPutChar(0x1b); return true;
        case '.': RawPutChar(0x19); return true;
        case 'a': RawPutChar(0xb1); return true;
        case 'f': RawPutChar(0xf8); return true;
        case 'g': RawPutChar(0xf1); return true;
        case 'h': RawPutChar(0xb0); return true;
        case 'j': RawPutChar(0xd9); return true;
        case 'k': RawPutChar(0xbf); return true;
        case 'l': RawPutChar(0xda); return true;
        case 'm': RawPutChar(0xc0); return true;
        case 'n': RawPutChar(0xc5); return true;
        case 'q': RawPutChar(0xc4); return true;
        case 's': RawPutChar(0x5f); return true;
        case 't': RawPutChar(0xc3); return true;
        case 'u': RawPutChar(0xb4); return true;
        case 'v': RawPutChar(0xc1); return true;
        case 'w': RawPutChar(0xc2); return true;
        case 'x': RawPutChar(0xb3); return true;
        case 'y': RawPutChar(0xf3); return true;
        case 'z': RawPutChar(0xf2); return true;
        case '~': RawPutChar(0xfa); return true;
        case '_': RawPutChar(0xff); return true;
        case '+': RawPutChar(0x1a); return true;
        case '{': RawPutChar(0xe3); return true;
        case '}': RawPutChar(0x9c); return true;
    }

    return false;
}
