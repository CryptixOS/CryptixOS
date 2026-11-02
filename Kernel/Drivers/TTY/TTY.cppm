/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/termios.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>

export module Cryptix:Drivers.TTY.TTY;

export import :Drivers.Core.CharacterDevice;
export import :Library.Locking.UniqueGuard;
export import :Scheduler.Event;

export class TTY : public CharacterDevice
{
  public:
    TTY(StringView name, usize minor);

    inline static TTY*     Active() { return s_CurrentTTY; }
    inline static TTY*     GetCurrent() { return s_CurrentTTY; }
    static ErrorOr<void>   SwitchTo(TTY* tty);

    bool                   GetCursorKeyMode() const;
    void                   SendBuffer(const char* string, usize bytes = 1);

    virtual StringView     Name() const noexcept override;
    winsize                GetSize() const;

    const termios2&        GetTermios() const { return m_Termios; }
    void                   SetTermios(const termios2& termios);

    virtual ErrorOr<File*> Open(File* file) override;
    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override;
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override;

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override;

    static void            Initialize();

  protected:
    virtual ErrorOr<void>  TransmitChar(u64 c) { return Error(ENOSYS); }
    virtual ErrorOr<isize> Transmit(StringView data) { return Error(ENOSYS); }

  private:
    static Vector<TTY*>     s_TTYs;
    static TTY*             s_CurrentTTY;

    StringView              m_Name = "tty"_sv;
    Spinlock                m_RawLock;
    Spinlock                m_OutputLock;

    Mutex                   m_RawMutex;
    Mutex                   m_OutputMutex;

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
