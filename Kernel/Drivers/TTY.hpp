/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>
#include <Drivers/Core/CharacterDevice.hpp>

#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/String/String.hpp>

#include <Scheduler/Event.hpp>

class TTY : public CharacterDevice
{
  public:
    TTY(StringView name, Terminal* terminal, usize minor);

    inline static TTY*     GetCurrent() { return s_CurrentTTY; }

    bool                   GetCursorKeyMode() const;
    void                   SendBuffer(const char* string, usize bytes = 1);

    virtual StringView     Name() const noexcept override;
    winsize                GetSize() const;

    const termios2&        GetTermios() const { return m_Termios; }
    void                   SetTermios(const termios2& termios);

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;

    virtual i32            IoCtl(usize request, uintptr_t argp) override;

    static void            Initialize();

  private:
    static Vector<TTY*>     s_TTYs;
    static TTY*             s_CurrentTTY;

    StringView              m_Name = "tty"_sv;
    Spinlock                m_RawLock;
    Spinlock                m_OutputLock;

    Terminal*               m_Terminal = nullptr;
    termios2                m_Termios;

    pid_t                   m_ControlSid = -1;
    gid_t                   m_Pgid       = 100;

    CircularQueue<u8, 4096> m_RawBuffer;
    Deque<String>           m_LineQueue;

    Event                   m_OnAddLine;
    Event                   m_RawEvent;

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
        ScopedLock guard(m_RawLock);

        usize      lineLength = m_RawBuffer.Size();
        String     line;
        line.Reserve(lineLength);
        while (!m_RawBuffer.Empty()) line += m_RawBuffer.Pop();
        line.ShrinkToFit();
        m_LineQueue.PushBack(line);

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
