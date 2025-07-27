/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

enum class SocketDomain
{
    eUnspecified = 0,
    eLocal       = 1,
    eIPv4        = 2,
    eIPv6        = 10,
};
enum class SocketType
{
    eStream   = 1,
    eDataGram = 2,
    eRaw      = 3,
};
enum class NetworkProtocol
{
    eTcp = 0
};

class Socket
{
  public:
    static Socket* Create(SocketDomain domain, SocketType type,
                          NetworkProtocol protocol);

    virtual isize  SetOption(i32 level, i32 option, const void*, usize count);

    Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol);

  protected:
    SocketDomain    m_Domain = SocketDomain::eUnspecified;
    SocketType      m_Type   = SocketType::eRaw;
    NetworkProtocol m_Protocol;
};
