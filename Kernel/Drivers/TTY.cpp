/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/signal.h>
#include <API/Posix/sys/ttydefaults.h>

#include <Arch/CPU.hpp>
#include <Arch/InterruptGuard.hpp>

#include <Debug/Config.hpp>
#include <Drivers/DeviceManager.hpp>
#include <Drivers/TTY.hpp>
#include <Drivers/Terminal.hpp>

#include <Prism/String/StringView.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

#include <VFS/DevTmpFs/DevTmpFs.hpp>
#include <VFS/VFS.hpp>

#include <algorithm>
#include <magic_enum/magic_enum.hpp>

Vector<TTY*> TTY::s_TTYs{};
TTY*         TTY::s_CurrentTTY = nullptr;

TTY::TTY(StringView name, Terminal* terminal, usize minor)
    : CharacterDevice("tty", MakeDevice(4, minor))
    , m_Name(name)
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

bool TTY::GetCursorKeyMode() const { return m_Terminal->GetCursorKeyMode(); }
void TTY::SendBuffer(const char* string, usize bytes)
{
    if (string[0] == '\e') m_State = State::eEscapeSequence;
    for (usize i = 0; i < bytes; i++)
    {
        char c = string[i];
        if (m_Termios.c_iflag & ISTRIP) c &= 0x7f;
        if (SignalsEnabled())
        {
            if (c == m_Termios.c_cc[VINTR])
            {
                SendSignal(SIGINT);
                continue;
            }
            else if (c == m_Termios.c_cc[VQUIT])
            {
                SendSignal(SIGQUIT);
                continue;
            }
            else if (c == m_Termios.c_cc[VSUSP])
            {
                SendSignal(SIGINT);
                continue;
            }
        }

        // Line Feed
        if (c == '\n' && m_Termios.c_iflag & INLCR) c = '\r';
        // Carriage Return
        else if (c == '\r')
        {
            if (m_Termios.c_iflag & IGNCR) continue;
            if (m_Termios.c_iflag & ICRNL) c = '\n';
        }

        if (IsCanonicalMode() && OnCanonChar(c)) continue;

        EnqueueChar(c);
        if (m_Termios.c_lflag & ECHO) Echo(c);
    }

    m_State = State::eNormal;
    m_RawEvent.Trigger();
}

StringView TTY::Name() const noexcept { return m_Name; }
winsize    TTY::GetSize() const { return m_Terminal->GetSize(); }

void       TTY::SetTermios(const termios2& termios)
{
#if CTOS_TTY_LOG_TERMIOS
    LogDebug("TTY: Setting termios to ->\n{}", termios);
#endif
    m_Termios = termios;
    m_RawBuffer.Clear();
}

ErrorOr<isize> TTY::Read(void* buffer, off_t offset, usize bytes)
{
    if (IsCanonicalMode())
    {
        m_OnAddLine.Await();

        ScopedLock    guard(m_RawLock);
        const String& line  = m_LineQueue.PopFrontElement();
        const usize   count = std::min(bytes, line.Size());
        return line.Copy(reinterpret_cast<char*>(buffer), count);
    }

    char* dest = reinterpret_cast<char*>(buffer);
    if (m_RawBuffer.Size() < bytes) m_RawEvent.Await();

    ScopedLock guard(m_RawLock);
    usize      count = std::min(m_RawBuffer.Size(), bytes);

    isize      nread = 0;
    while (count--)
    {
        auto value = m_RawBuffer.Pop();

        if (value == 0x04) LogError("EOF: {:#x}", value);
        *dest++ = value;
        ++nread;
    }

    return nread;

    // isize            nread = src.copy(dest, count);
    // for (; count > 0; count--) m_RawBuffer.pop_front();
    while (count > 0 && m_RawBuffer.Size())
    {
        ScopedLock guard(m_RawLock);
        u8         ch = m_RawBuffer.Front();
        m_RawBuffer.Pop();
        *dest++ = ch;
        ++nread;
        --count;
    }

    return nread;
}
ErrorOr<isize> TTY::Write(const void* src, off_t offset, usize bytes)
{
    const char*                 s = reinterpret_cast<const char*>(src);
    static constexpr StringView MLIBC_LOG_SIGNATURE = "[mlibc]: "_sv;

    StringView                  str(s, bytes);

    if (str.StartsWith(MLIBC_LOG_SIGNATURE))
    {
        str.RemovePrefix(MLIBC_LOG_SIGNATURE.Size());
        LogMessage("[{}mlibc{}]: {}", AnsiColor::FOREGROUND_MAGENTA,
                   AnsiColor::FOREGROUND_WHITE, str);

        return bytes;
    }

    ScopedLock guard(m_OutputLock);
    m_Terminal->PrintString(str);
    return bytes;
}

ErrorOr<isize> TTY::Read(const UserBuffer& out, usize count, isize offset)
{
    return Read(out.Raw(), offset, count);
}
ErrorOr<isize> TTY::Write(const UserBuffer& in, usize count, isize offset)
{
    return Write(in.Raw(), offset, count);
}

i32 TTY::IoCtl(usize request, uintptr_t argp)
{
    if (!argp) return_err(-1, EFAULT);
    Process* current = Process::Current();

    switch (request)
    {
        case TCGETS:
        {
            std::memcpy(reinterpret_cast<void*>(argp), &m_Termios,
                        sizeof(m_Termios));
            break;
        }
        case TCSETS: SetTermios(*reinterpret_cast<termios2*>(argp)); break;
        case TCSETSW:
            // TODO(v1tr10l7): Drain the output buffer
            SetTermios(*reinterpret_cast<termios2*>(argp));
            break;
        case TCSETSF:
            // TODO(v1tr10l7): Allow current output buffer to drain
            SetTermios(*reinterpret_cast<termios2*>(argp));
            FlushInput();
            break;

        case TCGETS2:
        {
            std::memcpy(reinterpret_cast<void*>(argp), &m_Termios,
                        sizeof(termios2));
            break;
        }
        case TCSETS2:
        {
            std::memcpy(&m_Termios, reinterpret_cast<void*>(argp),
                        sizeof(termios2));
            break;
        }
        case TCSETSW2:
        {
            // TODO(v1tr10l7): Drain the output buffer
            std::memcpy(&m_Termios, reinterpret_cast<void*>(argp),
                        sizeof(termios2));
            break;
        }
        case TCSETSF2:
        {
            // TODO(v1tr10l7): Allow current output buffer to drain,
            //  and discard the input buffer
            std::memcpy(&m_Termios, reinterpret_cast<void*>(argp),
                        sizeof(termios2));
            break;
        }

        case TCGETA: return_err(-1, ENOSYS); break;
        case TCSETA: return_err(-1, ENOSYS); break;
        case TCSETAW: return_err(-1, ENOSYS); break;
        case TCSETAF: return_err(-1, ENOSYS); break;

        case TIOCGLCKTRMIOS: return_err(-1, ENOSYS); break;
        case TIOCSLCKTRMIOS: return_err(-1, ENOSYS); break;

        case TIOCGWINSZ: *reinterpret_cast<winsize*>(argp) = GetSize(); break;
        case TIOCSWINSZ: return_err(-1, ENOSYS); break;

        case TIOCINQ: *reinterpret_cast<u32*>(argp) = m_RawBuffer.Size(); break;

        case TIOCGETD: *reinterpret_cast<u32*>(argp) = m_Termios.c_line; break;
        case TIOCSETD: m_Termios.c_line = *reinterpret_cast<u32*>(argp); break;
        // Make the TTY the controlling terminal of the calling process
        case TIOCSCTTY:
            if (current->Sid() != current->Pid() || current->TTY())
                return_err(-1, EINVAL);
            if (m_ControlSid && current->Credentials().uid != 0)
                return_err(-1, EPERM);
            current->SetTTY(this);
            m_ControlSid = current->Credentials().sid;
            break;
        case TIOCNOTTY:
            if (current->TTY() != this
                || m_ControlSid != current->Credentials().sid)
                return_err(-1, EINVAL);

            current->SetTTY(nullptr);
            m_ControlSid = -1;

            // TODO(v1tr10l7): Send SIGHUP and SIGCONT to everyone in the
            // process group
            if (current->IsSessionLeader())
            {
                current->SendSignal(SIGHUP);
                current->SendSignal(SIGCONT);
            }
            break;

        // Get the pgid of the foreground process on this terminal
        case TIOCGPGRP: *reinterpret_cast<i32*>(argp) = m_Pgid; break;
        // Set the foreground process group ID of this terminal
        case TIOCSPGRP:
        {
            auto pgid = *reinterpret_cast<i32*>(argp);
            if (pgid < 0) return_err(-1, EINVAL);
            m_Pgid = pgid;
            break;
        }
        // Get the session ID
        case TIOCGSID: *reinterpret_cast<pid_t*>(argp) = m_ControlSid; break;

        default:
            LogWarn("TTY: Request not implemented: {:#x}, argp: {}", request,
                    argp);
            return_err(-1, EINVAL);
    }

    return 0;
}

void TTY::Initialize()
{
    AssertPMM_Ready();

    auto& terminals = Terminal::EnumerateTerminals();

    usize minor     = 1;
    for (auto& terminal : terminals)
    {
        LogTrace("TTY: Creating device /dev/tty{}...", minor);

        auto tty = new TTY(fmt::format("tty{}", minor).data(), terminal, minor);
        s_TTYs.PushBack(tty);

        auto result = DeviceManager::RegisterCharDevice(tty);
        if (!result) delete tty;
        minor++;
    }

    if (s_TTYs.Empty())
    {
        LogTrace("TTY: Creating device /dev/tty...");
        auto tty = new TTY("tty", Terminal::GetPrimary(), 0);
        s_TTYs.PushBack(tty);

        auto registered = DeviceManager::RegisterCharDevice(tty);
        if (!registered) delete tty;
    }
    if (!s_TTYs.Empty())
        VFS::MkNod("/dev/tty"_sv, 0644 | S_IFCHR, s_TTYs.Front()->ID());

    LogInfo("TTY: Initialized");
}

void TTY::SendSignal(i32 signal)
{
    if (m_Pgid == 0) return;
    InterruptGuard guard(false);

    LogDebug("TTY: Sending signal to everyone in '{}' process group", m_Pgid);
    Process::SendGroupSignal(m_Pgid, signal);
    return;

    Process* current = Process::Current();
    if (signal == SIGINT) current->Exit(0);

    Process* groupLeader = Scheduler::GetProcess(m_Pgid);
    groupLeader->SendSignal(signal);
    for (const auto& child : groupLeader->Children()) child->SendSignal(signal);
}

bool TTY::OnCanonChar(char c)
{
    if (m_State == State::eEscapeSequence)
    {
        EchoRaw(c);
        return true;
    }

    if (c == m_Termios.c_cc[VEOF])
    {
        AddLine();
        return true;
    }
    if (c == m_Termios.c_cc[VKILL] && m_Termios.c_lflag & ECHOK)
    {
        KillLine();
        return true;
    }
    if ((c == m_Termios.c_cc[VERASE] || c == '\b') && m_Termios.c_lflag & ECHOE)
    {
        EraseChar();
        return true;
    }
    if (c == m_Termios.c_cc[VWERASE] && m_Termios.c_lflag & ECHOE)
    {
        EraseWord();
        return true;
    }
    if (c == '\n')
    {
        if (m_Termios.c_lflag & ECHO || m_Termios.c_lflag & ECHONL) Echo(c);

        EnqueueChar(c);
        AddLine();
        return true;
    }
    if (c == m_Termios.c_cc[VEOL])
    {
        AddLine();
        // return true;
    }

    return false;
}
bool TTY::OnEscapeChar(char c) { return true; }

void TTY::EnqueueChar(u64 c)
{
    ScopedLock guard(m_RawLock);

#define TTY_DEBUG 0
#if TTY_DEBUG == 1
    LogDebug("raw: {:#x}", c);
#endif
    m_RawBuffer.Push(c);
}

void TTY::FlushInput()
{
    ScopedLock rawGuard(m_RawLock);

    m_RawBuffer.Clear();
}
void TTY::Echo(u64 c)
{
    if (!(m_Termios.c_oflag & OPOST)) return EchoRaw(c);

    if (c == '\n' && m_Termios.c_oflag & ONLCR) EchoRaw('\r');
    if (c == '\r' && m_Termios.c_oflag & ONLRET) return;

    EchoRaw(c);
}
void TTY::EchoRaw(u64 c) { m_Terminal->PutChar(c); }

void TTY::EraseChar()
{
    Assert(IsCanonicalMode());
    ScopedLock guard(m_RawLock);

    if (m_RawBuffer.Empty()) return;
    usize count = 1;
    if (IsControl(m_RawBuffer.PopFront())) count = 2;
    while (count-- && m_Termios.c_lflag & (ECHO | ECHOK))
    {
        EchoRaw('\b');
        EchoRaw(' ');
        EchoRaw('\b');
    }
}
void TTY::EraseWord()
{
    while (!m_RawBuffer.Empty() && m_RawBuffer.Back() != ' ') EraseChar();
    while (!m_RawBuffer.Empty() && m_RawBuffer.Back() == ' ') EraseChar();
}
