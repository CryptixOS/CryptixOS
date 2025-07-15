
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
#include <Drivers/Serial.hpp>
#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>
#include <Drivers/Video/VideoTerminal.hpp>

#include <Memory/PMM.hpp>
#include <Prism/String/StringUtils.hpp>

Vector<Terminal*>                Terminal::s_Terminals = {};
Span<Framebuffer, DynamicExtent> s_Framebuffers;

Terminal::Terminal()
{
    if (s_Terminals.Empty()) s_Terminals.PushBack(this);
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

isize Terminal::PrintString(StringView str)
{
    if (!m_Initialized) return 0;

    isize nwritten = 0;
    for (auto c : str) PutCharImpl(c), ++nwritten;
    Flush();

    return nwritten;
}

void Terminal::Bell()
{
#ifdef CTOS_TARGET_X86_64
    PCSpeaker::ToneOn(1000);
    IO::Delay(1000);
    PCSpeaker::ToneOff();
#endif
}

void Terminal::SetupFramebuffers(Span<Framebuffer, DynamicExtent> framebuffers)
{
    static bool s_Initialized = false;
    if (s_Initialized) return;

    s_Framebuffers = framebuffers;
    s_Initialized  = true;
}

Framebuffer& Terminal::PrimaryFramebuffer()
{
    Assert(!s_Framebuffers.Empty());
    return s_Framebuffers[0];
}
Span<Framebuffer> Terminal::Framebuffers() { return s_Framebuffers; }

Terminal*         Terminal::GetPrimary()
{
    return !s_Terminals.Empty() ? s_Terminals[0] : nullptr;
}
const Vector<Terminal*>& Terminal::EnumerateTerminals()
{
    if (s_Framebuffers.Empty())
    {
        LogWarn("Terminal: Can't to enumerate, no availabe framebuffers");
        return s_Terminals;
    }
    if (!s_Terminals.Empty()) return s_Terminals;

    LogTrace("Terminal: Initializing terminals for {} framebuffers...", s_Framebuffers.Size());
    for (auto& framebuffer : s_Framebuffers) 
    {
        auto terminal = VideoTerminal::Create(framebuffer);
        s_Terminals.PushBack(terminal);

        LogTrace("Terminal: Instantiated a terminal");
    }

    LogInfo("Terminal: Initialized {} terminals", s_Terminals.Size());
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
        m_EscapeValues[m_EscapeValueCount] = 1;

        ++m_EscapeValueCount;
        return;
    }
    u32 param   = m_EscapeValues[0];

    auto [x, y] = GetCursorPos();
    switch (c)
    {
        case '@': ICH(param); break;
        case '?': m_DecPrivate = true; return;
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
        case 'B': break;
        case 'C':
        {
            if (param + x > m_Size.ws_col - 1) param = m_Size.ws_col - 1 - x;
            SetCursorPos(x + param, y);
            break;
        }
        case 'D':
        {
            if (param > x) param = x;
            SetCursorPos(x - param, y);
            break;
        }
        case 'E':
        case 'F':
            LogWarn("Terminal: Sequence 'ESC[{:c}' is not implemented yet", c);
            break;
        case 'G': CHA(param); break;
        case 'H': CUP(); break;
        case 'J': ED(param); break;
        case 'K': EL(param); break;
        case 'L':
        case 'M':
        case 'P':
        case 'X':
        case 'a':
            LogWarn("Terminal: Sequence 'ESC[{:c}' is not implemented yet", c);
        case 'c':
        {
            break;
            TTY* tty = TTY::GetCurrent();
            if (!tty) break;

            StringView response   = "\e[?6c";

            auto&      termios    = const_cast<termios2&>(tty->GetTermios());
            auto       oldTermios = termios;
            m_Lock.Release();
            termios.c_lflag &= ~(ECHO | ECHOE | ECHOKE | ECHOK | ECHONL);
            tty->SendBuffer(response.Raw(), response.Size() + 1);
            m_Lock.Acquire();
            termios = oldTermios;
            break;
        }
        case 'd': VPA(param); break;
        case 'e':
        case 'f':
        case 'g':
            LogWarn("Terminal: Sequence 'ESC[{:c}' is not implemented yet", c);
            break;
        case 'h':
        case 'l':
            if (m_DecPrivate && m_EscapeValueCount > 0
                && m_EscapeValues[0] == 1)
                m_CursorKeyMode = c == 'h';
            break;
        case 'm': SGR(param); break;
        case 'n':
        {
            TTY* tty = TTY::GetCurrent();
            if (!tty) break;

            StringView response = "";
            if (param == 5) response = "\e[0n";
            else if (param == 6)
                response = fmt::format("\e[{};{}", x, y).data();

            auto& termios    = const_cast<termios2&>(tty->GetTermios());
            auto  oldTermios = termios;
            m_Lock.Release();
            termios.c_lflag &= ~(ECHO | ECHOE | ECHOKE | ECHOK | ECHONL);
            tty->SendBuffer(response.Raw(), response.Size());
            m_Lock.Acquire();
            termios = oldTermios;
            break;
        }
        case 'q':
        case 'r':
        case 's':
        case 'u':
        case '`':
            LogWarn("Terminal: Sequence 'ESC[{:c}' is not implemented yet", c);
            break;
        case ']':
            LogWarn(
                "Terminal: Linux Console Private CSI Sequences are currently "
                "not implemented");
            break;
    }

    m_State      = State::eNormal;
    m_DecPrivate = false;
}

void Terminal::Reset()
{
    m_TabSize            = 8;
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

void Terminal::ICH(u64 count)
{
    auto [x, y] = GetCursorPos();
    for (isize i = m_Size.ws_col - 1; i >= static_cast<isize>(x); i--)
    {
        MoveCharacter(i + count, y, i, y);
        SetCursorPos(i, y);
        RawPutChar(' ');
    }
    SetCursorPos(x, y);
}

void Terminal::CHA(u64 column)
{
    --column;
    if (column >= m_Size.ws_col) column = m_Size.ws_col - 1;

    auto y = GetCursorPos().second;
    SetCursorPos(column, y);
}
void Terminal::CUP()
{
    usize x = m_EscapeValues[1];
    usize y = m_EscapeValues[0];

    if (x > 0) --x;
    if (x >= m_Size.ws_col) x = m_Size.ws_col - 1;

    if (y > 0) --y;
    if (y >= m_Size.ws_row) y = m_Size.ws_row - 1;

    SetCursorPos(x, y);
}

void Terminal::ED(u64 parameter)
{
    auto [startX, startY] = GetCursorPos();
    if (m_EscapeValueCount == 0)
    {
        usize displaySize = m_Size.ws_col * m_Size.ws_row;
        usize currentPos  = startY * m_Size.ws_row + startX;

        for (usize i = currentPos; i < displaySize; i++) RawPutChar(' ');
        SetCursorPos(startX, startY);
    }
    if (parameter == 1)
    {
        SetCursorPos(0, startY);
        for (usize x = 0; x < startX; x++) RawPutChar(' ');
    }
    else if (parameter == 2 || parameter == 3)
    {
        Clear(0, false);
        SetCursorPos(0, 0);
    }
}
void Terminal::EL(u64 parameter)
{
    auto [x, y]  = GetCursorPos();
    usize startX = x;
    usize endX   = m_Size.ws_col;
    if (parameter == 1)
    {
        startX = 0;
        endX   = x;
    }
    else if (parameter == 2) startX = 0;

    SetCursorPos(startX, y);

    for (usize x = startX; x < endX; x++) RawPutChar(' ');
    if (parameter != 1) SetCursorPos(x, y);
}
void Terminal::VPA(u64 row)
{
    --row;
    if (row >= m_Size.ws_row) row = m_Size.ws_row - 1;

    auto x = GetCursorPos().first;
    SetCursorPos(x, row);
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
