/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/Logger.hpp>

#include <Network/IPv4Socket.hpp>
#include <Network/LocalSocket.hpp>
#include <Network/NetLinkSocket.hpp>
#include <Network/Socket.hpp>

#include <Scheduler/Process.hpp>

Socket::Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol)
    : m_Domain(domain)
    , m_Type(type)
    , m_Protocol(protocol)
{
}

ErrorOr<Socket*> Socket::Create(SocketDomain domain, SocketType type,
                                NetworkProtocol protocol)
{
    if (domain < SocketDomain::eUnspecified || domain >= SocketDomain::eCount)
        return Error(EAFNOSUPPORT);
    if (type < SocketType::eStream || type >= SocketType::eCount)
        return Error(EINVAL);

    // FIXME(v1tr10l7): Check if socket already exists

    switch (domain)
    {
        case SocketDomain::eLocal:
            return LocalSocket::Create(domain, type, protocol);
        case SocketDomain::eNetLink:
            return NetLinkSocket::Create(
                domain, type, static_cast<NetLinkProtocol>(protocol));

        default:
            LogError("Socket: Unsupported socket domain!");
            errno = ENOSYS;
            break;
    }

    return nullptr;
}
ErrorOr<::Ref<Socket>> Socket::Get(isize sockFdNum)
{
    auto                  process = Process::Current();
    ::Ref<FileDescriptor> sockFd
        = TryOrRet(process->GetFileDescriptor(sockFdNum));
    if (!sockFd)
        LogError("Socket::Get: No file descriptor found for fd number {:#x}",
                 sockFdNum);
    if (!sockFd->IsSocket())
        LogError(
            "Socket::Get: File descriptor at fd number {:#x} is not a socket!",
            sockFdNum);
    if (!sockFd->File())
        LogError(
            "Socket::Get: File descriptor at fd number {:#x} has no associated "
            "file!",
            sockFdNum);

    if (!sockFd || !sockFd->IsSocket() || !sockFd->File())
        return Error(ENOTSOCK);

    return reinterpret_cast<class Socket*>(sockFd->File());
}

ErrorOr<void> Socket::QueueConnectionFrom(Socket* peer)
{
    if (m_Pending.Size() >= m_BackLog) return Error(ECONNREFUSED);
    m_Pending.PushBack(peer);

    return {};
}
