/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/IPv4Socket.hpp>
#include <Network/NetLinkSocket.hpp>
#include <Network/Socket.hpp>

#include <Library/Logger.hpp>

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
