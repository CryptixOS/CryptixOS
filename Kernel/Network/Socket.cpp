/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/IPv4Socket.hpp>
#include <Network/Socket.hpp>

#include <Library/Logger.hpp>

Socket* Socket::Create(SocketDomain domain, SocketType type,
                       NetworkProtocol protocol)
{
    switch (domain)
    {
        case SocketDomain::eIPv4: return IPv4Socket::Create(type, protocol);

        default:
            LogError("Socket: Unsupported socket domain!");
            errno = ENOSYS;
            break;
    }

    return nullptr;
}

Socket::Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol)
    : m_Domain(domain)
    , m_Type(type)
    , m_Protocol(protocol)
{
}
