/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/NetLinkSocket.hpp>

NetLinkSocket::NetLinkSocket(SocketDomain domain, SocketType type,
                             NetLinkProtocol protocol)
    : Socket(domain, type, static_cast<NetworkProtocol>(protocol))
{
}

ErrorOr<NetLinkSocket*> NetLinkSocket::Create(SocketDomain    domain,
                                              SocketType      type,
                                              NetLinkProtocol protocol)
{
    if (type != SocketType::eRaw && type != SocketType::eDataGram)
        return Error(ESOCKTNOSUPPORT);

    if (protocol < NetLinkProtocol::eRoute || protocol >= NetLinkProtocol::eMax)
        return Error(EPROTONOSUPPORT);

    return new NetLinkSocket(domain, type, protocol);
}
