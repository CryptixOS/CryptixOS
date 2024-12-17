/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "TTY.hpp"

#include "Arch/CPU.hpp"

#include "Drivers/Terminal.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

#include "VFS/DevTmpFs/DevTmpFs.hpp"
#include "VFS/VFS.hpp"

namespace
{
    std::vector<TTY*> s_TTYs;
    constexpr usize   MAX_CHAR_BUFFER = 64;
} // namespace

TTY* TTY::s_CurrentTTY = nullptr;

TTY::TTY(Terminal* terminal, usize minor)
    : Device(DriverType::eTTY, static_cast<DeviceType>(minor))
    , terminal(terminal)
{
    if (!s_CurrentTTY) s_CurrentTTY = this;
}

void TTY::PutChar(char c)
{
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

    spinlock.Acquire();
    while (bytes > 0 && charBuffer.size() > 0)
    {
        (void)offset;
        *d++ = charBuffer.front();
        charBuffer.pop_front();

        --bytes;
        ++nread;
    }
    spinlock.Release();

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

    terminal->PrintString(str);
    return bytes;
}

i32 TTY::IoCtl(usize request, uintptr_t argp)
{
    if (!argp) return EINVAL;

    switch (request)
    {
        case TCGETS:
        {
            std::memcpy(reinterpret_cast<void*>(argp), &termios,
                        sizeof(termios));
            break;
        }
        case TCSETS:
        {
            std::memcpy(&termios, reinterpret_cast<void*>(argp),
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
            controlSid = CPU::GetCurrentThread()->parent->credentials.sid;
            break;
        case TIOCNOTTY: controlSid = -1; break;

        default: return ENOTTY;
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
        VFS::MkNod(VFS::GetRootNode(), "/dev/tty0", 0666, s_TTYs[0]->GetID());

    LogInfo("TTY: Initialized");
}
