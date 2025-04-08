/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>
#include <Drivers/Device.hpp>

#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Library/Spinlock.hpp>
#include <Scheduler/Event.hpp>

#include <deque>
#include <vector>

class TTY : public Device
{
  public:
    TTY(Terminal* terminal, usize minor);

    inline static TTY*       GetCurrent() { return s_CurrentTTY; }

    bool                     GetCursorKeyMode() const;
    void                     SendBuffer(const char* string, usize bytes = 1);

    virtual std::string_view GetName() const noexcept override { return "tty"; }
    winsize                  GetSize() const;

    void                     SetTermios(const termios2& termios);

    virtual isize Read(void* dest, off_t offset, usize bytes) override;
    virtual isize Write(const void* src, off_t offset, usize bytes) override;

    virtual i32   IoCtl(usize request, uintptr_t argp) override;

    static void   Initialize();

  private:
    static std::vector<TTY*> s_TTYs;
    static TTY*              s_CurrentTTY;

    Spinlock                 m_RawLock;
    Spinlock                 m_OutputLock;

    Terminal*                m_Terminal = nullptr;
    termios2                 m_Termios;

    pid_t                    m_ControlSid = -1;
    gid_t                    m_Pgid       = 100;

    CircularQueue<u8, 4096>  m_RawBuffer;
    // std::deque<char>         m_RawBuffer;
    std::deque<std::string>  m_LineQueue;

    Event                    m_OnAddLine;
    Event                    m_RawEvent;

    enum class State
    {
        eNormal          = 0,
        eEscapeSequence  = 1,
        eControlSequence = 2,
        eDecPrivate      = 4,
    };
    State       m_State = State::eNormal;

    void        SendSignal(i32 signal);

    inline bool IsCanonicalMode() const { return m_Termios.c_lflag & ICANON; }
    inline bool SignalsEnabled() const { return m_Termios.c_lflag & ISIG; }

    bool        OnCanonChar(char c);
    bool        OnEscapeChar(char c);

    inline void AddLine()
    {
        ScopedLock  guard(m_RawLock);

        usize       lineLength = m_RawBuffer.Size();
        std::string line;
        line.reserve(lineLength);
        while (!m_RawBuffer.Empty()) line += m_RawBuffer.Pop();
        line.shrink_to_fit();
        m_LineQueue.push_back(line);

        m_RawBuffer.Clear();
        m_OnAddLine.Trigger();
    }
    inline void KillLine()
    {
        ScopedLock guard(m_RawLock);
        m_RawBuffer.Clear();
    }

    void           EnqueueChar(u64 c);
    void           EnqueueRawChar(u64 c);
    void           EnqueueCanonChar(u64 c);

    void           FlushInput();

    void           Echo(u64 c);
    void           EchoRaw(u64 c);

    void           EraseChar();
    void           EraseWord();

    constexpr bool IsControl(char c) const { return c < ' ' || c == 0x7f; }
};
