/*
 * Created by v1tr10l7 on 25.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/Socket.hpp>

class IPv4Socket : public Socket
{
  public:
    static Socket* Create(SocketType type, NetworkProtocol protocol);

    IPv4Socket(SocketType type, NetworkProtocol protocol);
};
