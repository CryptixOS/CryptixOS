/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/signal.h>
#include <API/Posix/sys/ttydefaults.h>

#include <Arch/CPU.hpp>

#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

#include <cctype>

std::vector<TTY*> TTY::s_TTYs{};
TTY*              TTY::s_CurrentTTY = nullptr;

TTY::TTY(Terminal* terminal, usize minor)
    : Device(DriverType::eTTY, static_cast<DeviceType>(minor))
    , m_Terminal(terminal)
{
    if (!s_CurrentTTY) s_CurrentTTY = this;

    std::memset(&m_Termios, 0, sizeof(m_Termios));
    m_Termios.c_iflag        = TTYDEF_IFLAG;
    m_Termios.c_oflag        = TTYDEF_OFLAG;
    m_Termios.c_lflag        = TTYDEF_LFLAG;
    m_Termios.c_cflag        = TTYDEF_CFLAG;
    m_Termios.c_ispeed       = TTYDEF_SPEED;
    m_Termios.c_ospeed       = TTYDEF_SPEED;

    m_Termios.c_cc[VINTR]    = CINTR;
    m_Termios.c_cc[VQUIT]    = CQUIT;
    m_Termios.c_cc[VERASE]   = CERASE;
    m_Termios.c_cc[VKILL]    = CKILL;
    m_Termios.c_cc[VEOF]     = CEOF;
    m_Termios.c_cc[VTIME]    = CTIME;
    m_Termios.c_cc[VMIN]     = CMIN;
    m_Termios.c_cc[VSWTC]    = CSWTC;
    m_Termios.c_cc[VSTART]   = CSTART;
    m_Termios.c_cc[VSTOP]    = CSTOP;
    m_Termios.c_cc[VSUSP]    = CSUSP;
    m_Termios.c_cc[VEOL]     = CEOL;
    m_Termios.c_cc[VREPRINT] = CREPRINT;
    m_Termios.c_cc[VDISCARD] = CDISCARD;
    m_Termios.c_cc[VWERASE]  = CWERASE;
    m_Termios.c_cc[VLNEXT]   = CLNEXT;
    m_Termios.c_cc[VEOL2]    = CEOL2;
}

void TTY::PutChar(char c)
{
    if (m_Termios.c_iflag & ISTRIP) c &= 0x7F;

    if ((m_Termios.c_lflag & ISIG) == ISIG)
    {
        if (c == m_Termios.c_cc[VINFO]) return SendSignal(SIGINFO);
        if (c == m_Termios.c_cc[VINTR]) return SendSignal(SIGINT);
        if (c == m_Termios.c_cc[VQUIT]) return SendSignal(SIGQUIT);
        if (c == m_Termios.c_cc[VSUSP]) return SendSignal(SIGTSTP);
    }

    if (c == '\r')
    {
        if (m_Termios.c_iflag & IGNCR) return;
        if (m_Termios.c_iflag & ICRNL) c = '\n';
    }

    if (IsCanonicalMode())
    {
        if (c == m_Termios.c_cc[VEOF]) return AddLine();
        if (c == m_Termios.c_cc[VKILL] && m_Termios.c_lflag & ECHOK)
            return KillLine();

        if ((c == m_Termios.c_cc[VERASE] || c == '\b')
            && m_Termios.c_lflag & ECHOE)
            return EraseChar();
        if (c == m_Termios.c_cc[VWERASE] && m_Termios.c_lflag & ECHOE)
            return EraseWord();
        if (c == '\n')
        {
            if (m_Termios.c_lflag & ECHO || m_Termios.c_lflag & ECHONL) Echo(c);

            EnqueueChar(c);
            return AddLine();
        }
        if (c == m_Termios.c_cc[VEOL]) AddLine();
    }

    EnqueueChar(c);
    if (m_Termios.c_lflag & ECHO) Echo(c);
}

isize TTY::Read(void* buffer, off_t offset, usize bytes)
{
    if (IsCanonicalMode())
    {
        while (m_LineQueue.empty()) Arch::Pause();
        ScopedLock         guard(m_Lock);

        const std::string& line  = m_LineQueue.pop_front_element();
        const usize        count = std::min(bytes, line.size());
        return line.copy(reinterpret_cast<char*>(buffer), count);
    }

    ScopedLock guard(m_Lock);

    isize      nread = 0;
    char*      dest  = reinterpret_cast<char*>(buffer);
    for (; bytes > 0 && !m_InputBuffer.empty(); nread++, dest++)
        *dest = m_InputBuffer.pop_front_element();
    return nread;
}
isize TTY::Write(const void* src, off_t offset, usize bytes)
{
    const char*           s = reinterpret_cast<const char*>(src);
    static constexpr char MLIBC_LOG_SIGNATURE[] = "[mlibc]: ";

    std::string_view      str(s, bytes);

    if (str.starts_with(MLIBC_LOG_SIGNATURE))
    {
        std::string_view errorMessage(s + sizeof(MLIBC_LOG_SIGNATURE) - 1 - 1);
        LogMessage("[{}mlibc{}]: {} ", AnsiColor::FOREGROUND_MAGENTA,
                   AnsiColor::FOREGROUND_WHITE, errorMessage);

        return bytes;
    }

    m_Terminal->PrintString(str);
    return bytes;
}

i32 TTY::IoCtl(usize request, uintptr_t argp)
{
    if (!argp) return_err(-1, EFAULT);

    switch (request)
    {
        case TCGETS:
        {
            std::memcpy(reinterpret_cast<void*>(argp), &m_Termios,
                        sizeof(termios));
            break;
        }
        case TCSETS:
        case TCSETSW:
            std::memcpy(&m_Termios, reinterpret_cast<void*>(argp),
                        sizeof(termios));
            break;
        case TIOCGWINSZ:
        {
            // if (!m_Terminal->GetContext()) return_err(-1, ENOTTY);

            winsize* windowSize   = reinterpret_cast<winsize*>(argp);
            windowSize->ws_row    = m_Terminal->GetContext()->rows;
            windowSize->ws_col    = m_Terminal->GetContext()->cols;
            windowSize->ws_xpixel = m_Terminal->GetFramebuffer().width;
            windowSize->ws_ypixel = m_Terminal->GetFramebuffer().height;
            break;
        }
        case TIOCGPGRP: *reinterpret_cast<i32*>(argp) = m_Pgid; break;
        case TIOCSPGRP: m_Pgid = *reinterpret_cast<i32*>(argp); break;
        case TIOCSCTTY:
            m_ControlSid
                = CPU::GetCurrentThread()->parent->GetCredentials().sid;
            break;
        case TIOCNOTTY: m_ControlSid = -1; break;

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

void TTY::SendSignal(i32 signal)
{
    LogTrace("Handling signal");
    if (m_Pgid < 0) return;

    Process* current = Process::GetCurrent();
    if (signal == SIGINT) current->Exit(0);

    Process* groupLeader = Scheduler::GetProcess(m_Pgid);
    groupLeader->SendSignal(signal);
    for (const auto& child : groupLeader->GetChildren())
        child->SendSignal(signal);
}

void TTY::EnqueueChar(u64 c)
{
    ScopedLock guard(m_Lock);

    m_InputBuffer.push_back(c);
}
void TTY::Echo(u64 c)
{
    if (c == '\n' && m_Termios.c_oflag & ONLCR) m_Terminal->PutChar('\r');
    if (c == '\r' && m_Termios.c_oflag & ONLRET) return;

    m_Terminal->PutChar(c);
}
void TTY::EraseChar()
{
    ScopedLock guard(m_Lock);

    if (m_InputBuffer.empty()) return;
    usize count = 1;
    if (IsControl(m_InputBuffer.pop_back_element())) count = 2;
    if (m_Termios.c_lflag & ECHO && m_Termios.c_lflag & ECHOK)
    {
        while (count--)
        {
            Echo('\b');
            Echo(' ');
            Echo('\b');
        }
    }
}
void TTY::EraseWord()
{
    while (!m_InputBuffer.empty() && m_InputBuffer.back() != ' ') EraseChar();
    while (!m_InputBuffer.empty() && m_InputBuffer.back() == ' ') EraseChar();
}
