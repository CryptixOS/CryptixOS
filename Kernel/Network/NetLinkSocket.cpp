/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/linux/netlink.h>
#include <Network/NetLinkSocket.hpp>
#include <Scheduler/Process.hpp>

static UnorderedMap<NetLinkProtocol, NetLinkProtocolInfo> s_NetLinkProtocols;

NetLinkSocket::NetLinkSocket(SocketDomain domain, SocketType type,
                             NetLinkProtocol protocol)
    : Socket(domain, type, static_cast<NetworkProtocol>(protocol))
    , m_NetLinkProtocol(protocol)
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
    auto        process = Process::Current();
    sockaddr_nl nladdr
        = CopyFromUser(*reinterpret_cast<const sockaddr_nl*>(addr));
    if (nladdr.nl_family != AF_NETLINK) return Error(EINVAL);

    if (nladdr.nl_groups)
    {
        if (!process->IsSuperUser()) return Error(EPERM);
        if (!s_NetLinkProtocols[m_NetLinkProtocol].Registered)
            return Error(ENOENT);

        if (m_Groups.Size() >= s_NetLinkProtocols[m_NetLinkProtocol].Groups)
            return {};
        m_Groups.Resize(s_NetLinkProtocols[m_NetLinkProtocol].Groups);
        // realloc groups
    }

    if (m_Pid && nladdr.nl_pid != m_Pid) return Error(EINVAL);
    else if (!m_Pid)
    {
        // Insert socket
        // autobind
    }

    if (!nladdr.nl_groups && (m_Groups.Empty() || !m_Groups[0])) return {};

    return Error(ENOSYS);
}
