/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Scheduler/Process.hpp>
#include <Time/Time.hpp>

#include <VFS/SynthFsINode.hpp>

SynthFsINode::SynthFsINode(INode* parent, std::string_view name,
                           Filesystem* filesystem, mode_t mode)
    : INode(parent, name, filesystem)
{
    auto process       = Process::GetCurrent();

    m_Stats.st_dev     = filesystem->GetDeviceID();
    m_Stats.st_ino     = filesystem->GetNextINodeIndex();
    m_Stats.st_nlink   = 1;
    m_Stats.st_mode    = mode;
    m_Stats.st_uid     = process ? process->GetCredentials().euid : 0;
    m_Stats.st_gid     = process ? process->GetCredentials().egid : 0;
    m_Stats.st_rdev    = 0;
    m_Stats.st_size    = 0;
    m_Stats.st_blksize = 512;
    m_Stats.st_blocks  = 0;

    m_Stats.st_atim    = Time::GetReal();
    m_Stats.st_mtim    = Time::GetReal();
    m_Stats.st_ctim    = Time::GetReal();

    if (parent && parent->GetStats().st_mode & S_ISGID)
    {
        m_Stats.st_gid = parent->GetStats().st_gid;
        m_Stats.st_mode |= S_ISGID;
    }

    if (S_ISREG(mode))
    {
        m_Stats.st_size = DEFAULT_SIZE;
        m_Buffer.Resize(m_Stats.st_size);
    }
}

isize SynthFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);
    bytes = std::min(bytes, m_Buffer.Size() - offset);

    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
    m_Buffer.Read(offset, reinterpret_cast<Byte*>(buffer), bytes);

    return bytes;
}

isize SynthFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    Assert(buffer);
    Assert(offset >= 0);

    ScopedLock guard(m_Lock);

    usize      newCapacity = m_Buffer.Size();
    while (offset + bytes >= m_Buffer.Size()) newCapacity *= 2;

    m_Buffer.Resize(newCapacity);
    if (!m_Buffer.Raw()) return_err(-1, ENOMEM);
    m_Stats.st_size   = m_Buffer.Size();
    m_Stats.st_blocks = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);

    m_Buffer.Write(offset, reinterpret_cast<const Byte*>(buffer), bytes);
    if (m_Filesystem->ShouldUpdateATime()) m_Stats.st_atim = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime()) m_Stats.st_mtim = Time::GetReal();

    return bytes;
}

ErrorOr<isize> SynthFsINode::Truncate(usize size)
{
    ScopedLock guard(m_Lock);
    if (size == m_Buffer.Size()) return 0;

    const Credentials& creds = Process::GetCurrent()->GetCredentials();
    if (!CanWrite(creds)) return Error(EPERM);

    usize prevSize = m_Buffer.Size();
    m_Buffer.Resize(size > prevSize ? size : prevSize);
    m_Buffer.Fill(prevSize, 0);

    if (m_Filesystem->ShouldUpdateCTime()) m_Stats.st_ctim = Time::GetReal();
    if (m_Filesystem->ShouldUpdateMTime()) m_Stats.st_mtim = Time::GetReal();

    m_Stats.st_size   = static_cast<off_t>(size);
    m_Stats.st_blocks = Math::DivRoundUp(m_Stats.st_size, m_Stats.st_blksize);

    return 0;
}

ErrorOr<void> SynthFsINode::ChMod(mode_t mode)
{
    m_Stats.st_mode &= ~0777;
    m_Stats.st_mode |= 0777 & 0777;

    return {};
}
