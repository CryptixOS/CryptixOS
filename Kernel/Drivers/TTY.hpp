/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>
#include <Drivers/Device.hpp>

#include <Utility/Spinlock.hpp>

#include <cctype>
#include <deque>
#include <vector>

class TTY : public Device
{
  public:
    TTY(Terminal* terminal, usize minor);

    inline static TTY*       GetCurrent() { return s_CurrentTTY; }

    void                     PutChar(char c);

    virtual std::string_view GetName() const noexcept override { return "tty"; }

    virtual isize Read(void* dest, off_t offset, usize bytes) override;
    virtual isize Write(const void* src, off_t offset, usize bytes) override;

    virtual i32   IoCtl(usize request, uintptr_t argp) override;

    static void   Initialize();

  private:
    static std::vector<TTY*> s_TTYs;
    static TTY*              s_CurrentTTY;

    Spinlock                 m_Lock;

    Terminal*                m_Terminal = nullptr;
    termios                  m_Termios;

    pid_t                    m_ControlSid = -1;
    gid_t                    m_Pgid       = 100;
    std::deque<char>         m_InputBuffer;
    std::deque<std::string>  m_LineQueue;

    inline bool IsCanonicalMode() const { return m_Termios.c_lflag & ICANON; }
    inline bool SignalsEnabled() const { return m_Termios.c_lflag & ISIG; }

    inline void AddLine()
    {
        ScopedLock guard(m_Lock);
        m_LineQueue.emplace_back(m_InputBuffer.begin(), m_InputBuffer.end());
        m_InputBuffer.clear();
    }
    inline void KillLine()
    {
        while (!m_LineQueue.empty()) EraseChar();
    }

    void           EnqueueChar(u64 c);
    void           Echo(u64 c);
    void           EraseChar();
    void           EraseWord();

    constexpr bool IsControl(char c) const { return c < ' ' || c == 0x7f; }

    inline void    Output(char c) const
    {
        if (c == '\n' && m_Termios.c_oflag & ONLCR)
        {
            LogMessage("\n\r");
            return;
        }
        if (c == '\r' && m_Termios.c_oflag & ONLRET) return;

        LogMessage("{}", c);
    }
};
