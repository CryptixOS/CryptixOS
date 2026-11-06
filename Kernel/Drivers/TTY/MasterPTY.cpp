/*
 * Created by v1tr10l7 on 08.02.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/asm/termbits.h>
#include <Arch/User.hpp>
#include <Drivers/TTY/MasterPTY.hpp>
#include <Drivers/TTY/SlavePTY.hpp>

MasterPTY::MasterPTY(::Ref<PTYPair> pair)
    : CharacterDevice("ptm"_sv, PTY::MASTER_MAJOR, pair->Index)
    , m_Pair(pair)
    , m_Name(fmt::format("ptm{}", pair->Index).data())
{
}

ErrorOr<isize> MasterPTY::Read(void* dest, off_t offset, usize bytes)
{
    if (bytes == 0) return 0;
    u8* out = reinterpret_cast<u8*>(dest);

    for (;;)
    {
        usize n = m_Pair->PopOutput(out, bytes);
        if (n) return static_cast<isize>(n);

        {
            ScopedLock guard(m_Pair->Lock);
            if (m_Pair->SlaveClosed == 0 && m_Pair->Output.Empty()) return 0;
        }

        // TODO(v1tr10l7): O_NONBLOCK: return Error(EAGAIN) here once the fd
        // flags reach Device::Read
        if (!m_Pair->OutputReady.Await(true)) return Error(EINTR);
    }
}
ErrorOr<isize> MasterPTY::Write(const void* src, off_t offset, usize bytes)
{
    if (!m_Pair->Slave) return Error(EIO);

    m_Pair->Slave->SendBuffer(reinterpret_cast<const char*>(src), bytes);
    return static_cast<isize>(bytes);
}

ErrorOr<isize> MasterPTY::IoCtl(usize request, upointer argp)
{
    switch (request)
    {
        case TIOCGPTN:
        {
            usize index  = m_Pair->Index;
            auto  result = CopyToUser(argp, &index, sizeof(index));
            if (!result) return result.Error();

            return 0;
        }
        case TIOCSPTLCK:
        {
            i32  lock   = 0;
            auto result = CopyFromUser(&lock, argp, sizeof(lock));
            if (!result) return result.Error();
            ScopedLock guard(m_Pair->Lock);
            m_Pair->Locked = (lock != 0);

            return 0;
        }
        case TIOCGWINSZ:
        case TIOCSWINSZ:
        case TCGETS:
        case TCSETS:
        case TCSETSW:
        case TCSETSF:
            if (!m_Pair->Slave) return Error(EIO);
            return m_Pair->Slave->IoCtl(request, argp);

        default: return Error(ENOTTY);
    }
}

void MasterPTY::Close(bool)
{
    {
        ScopedLock guard(m_Pair->Lock);
        if (m_Pair->MasterOpens) --m_Pair->MasterOpens;
        if (m_Pair->MasterOpens != 0) return;
    }
    m_Pair->OnMasterClosed();
}
