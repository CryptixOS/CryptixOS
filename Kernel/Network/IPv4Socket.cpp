/*
 * Created by v1tr10l7 on 25.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/IPv4Socket.hpp>

Socket* IPv4Socket::Create(SocketType type, NetworkProtocol protocol)
{
    return new IPv4Socket(type, protocol);
}

IPv4Socket::IPv4Socket(SocketType type, NetworkProtocol protocol)
    : Socket(SocketDomain::eIPv4, type, protocol)
{
}
