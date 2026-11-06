/*
 * Created by v1tr10l7 on 05.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/TTY/MasterPTY.hpp>
#include <Drivers/TTY/SlavePTY.hpp>
#include <Prism/String/StringUtils.hpp>

SlavePTY::SlavePTY(::Ref<PTYPair> pair)
    : TTY("pts"_sv, PTY::SLAVE_MAJOR, pair->Index)
    , m_Pair(pair)
    , m_Name(StringUtils::ToString(pair->Index))
{
    // Starts at 0x0 until the master sets it, like Linux
    UseOwnWindowSize();
}

ErrorOr<File*> SlavePTY::Open(File* file)
{
    ScopedLock guard(m_Pair->Lock);
    if (m_Pair->Locked) return Error(EIO);
    if (m_Pair->MasterClosed) return Error(EIO);

    ++m_Pair->SlaveOpens;
    return file;
}

void SlavePTY::Close(bool)
{
    {
        ScopedLock guard(m_Pair->Lock);
        if (m_Pair->SlaveOpens) --m_Pair->SlaveOpens;
        if (m_Pair->SlaveOpens) return;
    }
    m_Pair->OnSlaveClosed();
}

ErrorOr<void> SlavePTY::TransmitChar(u64 c)
{
    u8 byte = static_cast<u8>(c);

    m_Pair->PushOutput(&byte, 1);
    return {};
}
ErrorOr<isize> SlavePTY::Transmit(StringView data)
{
    const auto& termios = GetTermios();
    bool onlcr = (termios.c_oflag & OPOST) && (termios.c_oflag & ONLCR);

    for (usize i = 0; i < data.Size(); ++i)
    {
        u8 chunk[2];
        usize len = 0;
        if (onlcr && data[i] == '\n') chunk[len++] = '\r';
        chunk[len++] = data[i];

        for (usize done = 0; done < len;)
        {
            {
                ScopedLock guard(m_Pair->Lock);
                if (m_Pair->MasterClosed) return Error(EIO);
            }
            done += m_Pair->PushOutput(&chunk[done], len - done);
            if (done < len && !m_Pair->OutputSpace.Await(true)) return Error(EINTR);
        }
    }
    
    return static_cast<isize>(data.Size());
}
void SlavePTY::Hangup()
{
    SendSignal(SIGHUP);
}
