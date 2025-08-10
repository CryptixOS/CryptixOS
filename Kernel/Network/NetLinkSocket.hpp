/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/Socket.hpp>

class NetLinkSocket : public Socket
{
  public:
    NetLinkSocket(SocketDomain domain, SocketType type,
                  NetLinkProtocol protocol);

    static ErrorOr<NetLinkSocket*> Create(SocketDomain domain, SocketType type,
                                          NetLinkProtocol protocol);
};
