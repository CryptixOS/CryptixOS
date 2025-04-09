
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
#include <Prism/String/StringUtils.hpp>

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
    if (m_State == State::eEscape)
    {
        OnEscapeChar(c);
        return;
    }
    else if (m_State == State::eControlSequenceEntry
             || m_State == State::eControlSequenceParams)
    {
        OnCsi(c);
        return;
    }

    auto [x, y] = GetCursorPos();
    switch (c)
    {
        case '\0': return;
        case '\a': Bell(); return;
        case '\b':
            if (x == 0) return;
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
        case '\v':
        case '\f':
            if (y == m_ScrollBottomMargin - 1)
            {
                ScrollDown();
                SetCursorPos(0, y);
            }
            else SetCursorPos(0, ++y);
            return;
        case '\r': SetCursorPos(0, y); return;
        case 0x0e:
            // Activate the G1 character set;
            return;
        case 0x0f:
            // Activate the G0 character set;
            return;
        case 0x1a: m_State = State::eNormal; return;
        case '\e': m_State = State::eEscape; return;
        case 0x7f: return;
        case 0x9b: m_State = State::eControlSequenceEntry; return;
    }

    // TODO(v1tr10l7): charset #1

    if (c >= 0x20 && c <= 0x7e) return RawPutChar(c);
    RawPutChar(0xfe);
}

void Terminal::PrintString(StringView str)
{
    if (!m_Initialized) return;

    for (auto c : str) PutCharImpl(c);
    Flush();
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
            m_State            = State::eControlSequenceEntry;
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
void Terminal::OnCsi(char c)
{
    if (std::isdigit(c))
    {
        m_State = State::eControlSequenceParams;

        if (m_EscapeValueCount == MAX_ESC_PARAMETER_COUNT) return;
        m_EscapeValues[m_EscapeValueCount] *= 10;
        m_EscapeValues[m_EscapeValueCount] += c - '0';
        return;
    }

    if (m_State == State::eControlSequenceParams)
    {
        ++m_EscapeValueCount;
        m_State = State::eControlSequenceEntry;
        if (c == ';') return;
    }
    else if (c == ';')
    {
        if (m_EscapeValueCount == MAX_ESC_PARAMETER_COUNT) return;
        m_EscapeValues[m_EscapeValueCount] = 0;
        ++m_EscapeValueCount;
        return;
    }
    u32 param   = m_EscapeValueCount > 0 ? m_EscapeValues[0] : 0;

    auto [x, y] = GetCursorPos();
    switch (c)
    {
        case '@':
        {
            for (usize i = m_Size.ws_col - 1; i--;)
            {
                MoveCharacter(i + param, y, i, y);
                SetCursorPos(i, y);
                RawPutChar(' ');
                if (i == x) break;
            }
            SetCursorPos(x, y);
            break;
        }
        case 'A':
        {
            if (param > y) param = y;
            usize origY          = y;
            usize destY          = y - param;
            bool  willBeScrolled = false;
            if ((m_ScrollTopMargin >= destY && m_ScrollTopMargin <= origY)
                || (m_ScrollBottomMargin >= destY
                    && m_ScrollBottomMargin <= origY))
                willBeScrolled = true;
            if (willBeScrolled && destY < m_ScrollTopMargin)
                destY = m_ScrollTopMargin;
            SetCursorPos(x, destY);
            break;
        }
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'P':
        case 'X':
        case 'a':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'l':
        case 'm': SGR(param); break;
        case 'n':
        case 'q':
        case 'r':
        case 's':
        case 'u':
        case '`':
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
        case '`': RawPutChar(0x04); break;
        case '0': RawPutChar(0xdb); break;
        case '-': RawPutChar(0x18); break;
        case ',': RawPutChar(0x1b); break;
        case '.': RawPutChar(0x19); break;
        case 'a': RawPutChar(0xb1); break;
        case 'f': RawPutChar(0xf8); break;
        case 'g': RawPutChar(0xf1); break;
        case 'h': RawPutChar(0xb0); break;
        case 'j': RawPutChar(0xd9); break;
        case 'k': RawPutChar(0xbf); break;
        case 'l': RawPutChar(0xda); break;
        case 'm': RawPutChar(0xc0); break;
        case 'n': RawPutChar(0xc5); break;
        case 'q': RawPutChar(0xc4); break;
        case 's': RawPutChar(0x5f); break;
        case 't': RawPutChar(0xc3); break;
        case 'u': RawPutChar(0xb4); break;
        case 'v': RawPutChar(0xc1); break;
        case 'w': RawPutChar(0xc2); break;
        case 'x': RawPutChar(0xb3); break;
        case 'y': RawPutChar(0xf3); break;
        case 'z': RawPutChar(0xf2); break;
        case '~': RawPutChar(0xfa); break;
        case '_': RawPutChar(0xff); break;
        case '+': RawPutChar(0x1a); break;
        case '{': RawPutChar(0xe3); break;
        case '}': RawPutChar(0x9c); break;

        default: return false;
    }

    return true;
}

void Terminal::SGR(u64 parameter)
{
    AnsiColor color
        = static_cast<AnsiColor>((parameter % 10) + (parameter >= 90 ? 10 : 0));

    // TODO(v1tr10l7): SGR attributes -> bold, half-bright, italic, underscore,
    // blink, reverse video, reset selected mapping, select null mapping,
    // underline, normal intensity
    switch (parameter)
    {
        // Reset all attributes to their defaults
        case 0:
            SetTextForeground(AnsiColor::eDefault);
            SetTextBackground(AnsiColor::eDefault);
            break;
        // set bold
        case 1: break;
        // set half-bright
        case 2: break;
        // set set italic
        case 3: break;
        // set underscore
        case 4: break;
        // set blink
        case 5: break;
        // set reverse video
        case 7: break;
        // reset selected mapping, display control flag, and toggle meta flag
        case 10: break;
        // select null mapping, set display control flag, reset toggle meta flag
        case 11: break;
        // select null mapping, set display control flag, set toggle meta flag
        case 12: break;
        // set underline
        case 21: break;
        // set normal intensity
        case 22: break;
        // italic off
        case 23: break;
        // underline off
        case 24: break;
        // blink off
        case 25: break;
        // reverse video off
        case 27: break;
        // set text foreground color
        case 30 ... 37:
        // set default text foreground color
        case 39:
        // set bright text foreground color
        case 90 ... 97: SetTextForeground(color); break;
        // set 256/24-bit text foreground color
        case 38:
        // set 256/24-bit text background color
        case 48:
        {
            if (m_EscapeValueCount < 4) break;
            auto  r = m_EscapeValues[1];
            auto  g = m_EscapeValues[2];
            auto  b = m_EscapeValues[3];
            Color rgb(r, g, b);

            if (parameter == 38) SetTextForegroundRgb(rgb);
            else if (parameter == 48) SetTextBackgroundRgb(rgb);
            break;
        }
        // set text background color
        case 40 ... 47:
        // set default text background color
        case 49:
        // set bright text background color
        case 100 ... 107: SetTextBackground(color); break;

        default: break;
    }
}
