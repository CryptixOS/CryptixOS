/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/Socket.hpp>

class NetLinkSocket;
struct NetLinkProtocolInfo
{
    bool                   Registered = false;
    usize                  Groups     = 0;
    Vector<NetLinkSocket*> BoundSockets;
};

class NetLinkSocket : public Socket
{
  public:
    NetLinkSocket(SocketDomain domain, SocketType type,
                  NetLinkProtocol protocol);

    static ErrorOr<NetLinkSocket*> Create(SocketDomain domain, SocketType type,
                                          NetLinkProtocol protocol);

    virtual ErrorOr<void>          Bind(const struct sockaddr* addr,
                                        socklen_t              len) override;

  private:
    NetLinkProtocol m_NetLinkProtocol;
    u64             m_Pid;
    Vector<usize>   m_Groups;
};
