/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>

#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

namespace
{
    std::vector<TTY*> s_TTYs;
    constexpr usize   MAX_CHAR_BUFFER = 64;
} // namespace
#define CTRL(c)  (c & 0x1F)
#define CINTR    CTRL('c')
#define CQUIT    034
#define CERASE   010
#define CKILL    CTRL('u')
#define CEOF     CTRL('d')
#define CTIME    0
#define CMIN     1
#define CSWTC    0
#define CSTART   CTRL('q')
#define CSTOP    CTRL('s')
#define CSUSP    CTRL('z')
#define CEOL     0
#define CREPRINT CTRL('r')
#define CDISCARD CTRL('o')
#define CWERASE  CTRL('w')
#define CLNEXT   CTRL('v')
#define CEOL2    CEOL

#define CEOT     CEOF
#define CBRK     CEOL
#define CRPRNT   CREPRINT
#define CFLUSH   CDISCARD

TTY*        TTY::s_CurrentTTY = nullptr;
static cc_t ttydefchars[NCCS];

TTY::TTY(Terminal* terminal, usize minor)
    : Device(DriverType::eTTY, static_cast<DeviceType>(minor))
    , terminal(terminal)
{
    if (!s_CurrentTTY) s_CurrentTTY = this;

    ttydefchars[VINTR]    = CINTR;
    ttydefchars[VQUIT]    = CQUIT;
    ttydefchars[VERASE]   = CERASE;
    ttydefchars[VKILL]    = CKILL;
    ttydefchars[VEOF]     = CEOF;
    ttydefchars[VTIME]    = CTIME;
    ttydefchars[VMIN]     = CMIN;
    ttydefchars[VSWTC]    = CSWTC;
    ttydefchars[VSTART]   = CSTART;
    ttydefchars[VSTOP]    = CSTOP;
    ttydefchars[VSUSP]    = CSUSP;
    ttydefchars[VEOL]     = CEOL;
    ttydefchars[VREPRINT] = CREPRINT;
    ttydefchars[VDISCARD] = CDISCARD;
    ttydefchars[VWERASE]  = CWERASE;
    ttydefchars[VLNEXT]   = CLNEXT;
    ttydefchars[VEOL2]    = CEOL2;

    std::memset(&m_Termios, 0, sizeof(m_Termios));
    m_Termios.c_iflag  = ICRNL;
    m_Termios.c_oflag  = OPOST | ONLCR;
    m_Termios.c_lflag  = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL;
    m_Termios.c_cflag  = CS8;
    m_Termios.c_ispeed = B9600;
    m_Termios.c_ospeed = B9600;
    std::memcpy(m_Termios.c_cc, ttydefchars, NCCS);
}

void TTY::PutChar(char c)
{
    Logger::LogChar(c);
    //  if (m_Termios.c_iflag & ISTRIP) c &= 0x7F;

    static std::deque<char> line;

    spinlock.Acquire();
    if (c == 0xa)
    {
        line.push_back(c);
        m_LineQueue.push_back(line);
        line.clear();
    }
    else line.push_back(c);
    spinlock.Release();

    return;

    // TODO(v1tr10l7): Check whether we should generate signals
    if (SignalsEnabled())
        ;

    if (c == '\r' && (m_Termios.c_iflag & ICRNL)) c = '\n';
    else if (c == '\n' && (m_Termios.c_iflag & INLCR)) c = '\r';

    if (m_IsNextVerbatim)
    {
        m_IsNextVerbatim = false;
        m_CanonQueue.push_back(c);
        if (m_Termios.c_lflag & ECHO)
        {
            if (IsControlCharacter(c))
            {
                Output('^');
                Output(('@' + c) % 128);
            }
            else Output(c);
        }

        return;
    }

    if (IsCanonicalMode())
    {
        // if (c == m_Termios.c_cc[VLNEXT] && m_Termios.c_lflag & IEXTEN)
        //   ;
    }

    spinlock.Acquire();
    if (charBuffer.size() > MAX_CHAR_BUFFER) charBuffer.pop_front();
    charBuffer.push_back(c);
    spinlock.Release();
}

isize TTY::Read(void* dest, off_t offset, usize bytes)
{
    isize nread = 0;
    Assert(dest);
    u8* d = reinterpret_cast<u8*>(dest);

    m_Termios.c_lflag |= ICANON;
    // LogInfo("BeforeYield");
    while (m_LineQueue.empty()) Arch::Pause(); // Scheduler::Yield();
    // LogInfo("AfterYield");

    spinlock.Acquire();

    auto& line = m_LineQueue.front();
    while (bytes > 0 && !line.empty())
    {
        char c = line.front();
        line.pop_front();

        *d++ = c;
        --bytes;
        ++nread;
    }
    m_LineQueue.pop_front();

    // LogInfo("Line: {}", line.data());
    spinlock.Release();

    /*
    while (bytes > 0 && charBuffer.size() > 0)
    {
        break;
        (void)offset;

        char c = charBuffer.front();

        *d++   = c;
        charBuffer.pop_front();

        --bytes;
        ++nread;
    }
    */

    (void)MAX_CHAR_BUFFER;
    return nread;
}
isize TTY::Write(const void* src, off_t offset, usize bytes)
{
    const char*           s = reinterpret_cast<const char*>(src);
    static constexpr char MLIBC_LOG_SIGNATURE[] = "[mlibc]: ";

    std::string_view      str(s + offset, bytes);

    if (str.starts_with(MLIBC_LOG_SIGNATURE))
    {
        std::string_view errorMessage(s + offset + sizeof(MLIBC_LOG_SIGNATURE)
                                      - 1 - 1);
        LogMessage("[{}mlibc{}]: {} ", AnsiColor::FOREGROUND_MAGENTA,
                   AnsiColor::FOREGROUND_WHITE, errorMessage);

        return bytes;
    }

    Logger::LogString(str);
    return bytes;
}

i32 TTY::IoCtl(usize request, uintptr_t argp)
{
    if (!argp) return EINVAL;

    switch (request)
    {
        case TCGETS:
        {
            std::memcpy(reinterpret_cast<void*>(argp), &m_Termios,
                        sizeof(termios));
            break;
        }
        case TCSETS:
        {
            std::memcpy(&m_Termios, reinterpret_cast<void*>(argp),
                        sizeof(termios));
            break;
        }
        case TIOCGWINSZ:
        {
            if (!terminal->GetContext()) return_err(-1, ENOTTY);

            winsize* windowSize   = reinterpret_cast<winsize*>(argp);
            windowSize->ws_row    = terminal->GetContext()->rows;
            windowSize->ws_col    = terminal->GetContext()->cols;
            windowSize->ws_xpixel = terminal->GetContext()->saved_cursor_x;
            windowSize->ws_ypixel = terminal->GetContext()->saved_cursor_y;
            break;
        }
        case TIOCGPGRP: *reinterpret_cast<i32*>(argp) = pgid; break;
        case TIOCSPGRP: pgid = *reinterpret_cast<i32*>(argp); break;
        case TIOCSCTTY:
            controlSid = CPU::GetCurrentThread()->parent->GetCredentials().sid;
            break;
        case TIOCNOTTY: controlSid = -1; break;

        default:
            LogInfo("Request: {:#x}, argp: {}", request, argp);
            return EINVAL;
    }

    return no_error;
}

void TTY::Initialize()
{
    AssertPMM_Ready();

    auto& terminals = Terminal::EnumerateTerminals();

    usize minor     = 1;
    for (usize i = 0; i < terminals.size(); i++)
    {
        LogTrace("TTY: Creating device /dev/tty{}...", minor);

        auto tty = new TTY(terminals[i], minor);
        s_TTYs.push_back(tty);
        DevTmpFs::RegisterDevice(tty);

        std::string path = "/dev/tty";
        path += std::to_string(minor);
        VFS::MkNod(VFS::GetRootNode(), path, 0666, tty->GetID());
        minor++;
    }

    if (!s_TTYs.empty())
    {
        VFS::MkNod(VFS::GetRootNode(), "/dev/tty0", 0666, s_TTYs[0]->GetID());
        VFS::MkNod(VFS::GetRootNode(), "/dev/tty", 0666, s_TTYs[0]->GetID());
        VFS::MkNod(VFS::GetRootNode(), "/dev/console", 0666,
                   s_TTYs[0]->GetID());
    }

    LogInfo("TTY: Initialized");
}
