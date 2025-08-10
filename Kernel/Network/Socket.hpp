/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Sockets.hpp>
#include <Prism/Core/Types.hpp>
#include <VFS/File.hpp>

class Socket : public File
{
  public:
    Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol);

    static ErrorOr<Socket*> Create(SocketDomain domain, SocketType type,
                                   NetworkProtocol protocol);

  protected:
    SocketDomain    m_Domain = SocketDomain::eUnspecified;
    SocketType      m_Type   = SocketType::eRaw;
    NetworkProtocol m_Protocol;
};
