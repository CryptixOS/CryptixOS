/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/linux/netlink.h>
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

ErrorOr<void> NetLinkSocket::Bind(const struct sockaddr* addr, socklen_t len)
{
    sockaddr_nl nladdr
        = CPU::CopyFromUser(*reinterpret_cast<const sockaddr_nl*>(addr));
    if (nladdr.nl_family != AF_NETLINK) return Error(EINVAL);

    return Error(ENOSYS);
}
