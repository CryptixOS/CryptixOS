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

enum class SocketState
{
    eUnconnected = 0,
    eConnected   = 1,
};
class Socket : public File
{
  public:
    Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol);

    static ErrorOr<Socket*>       Create(SocketDomain domain, SocketType type,
                                         NetworkProtocol protocol);
    static ErrorOr<::Ref<Socket>> Get(isize sockFdNum);

    virtual ErrorOr<void>         Connect(const struct sockaddr* addr,
                                          socklen_t              addrlen)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> Accept(sockaddr* addr, socklen_t* addrlen)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> SendTo(u8* data, usize size, isize flags)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> ReceiveFrom(u8* data, usize size, isize flags,
                                       sockaddr* saddr, socklen_t* addrlen)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> Bind(const struct sockaddr* addr, socklen_t addrlen)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> Listen(i32 backlog) { return Error(ENOSYS); }

    virtual ErrorOr<void> GetLocalAddress(sockaddr* addr, socklen_t* addrlen)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> GetPeerAddress(sockaddr* addr, socklen_t* addrlen)
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> GetOption(isize level, isize option, u8* out,
                                    socklen_t* outSize)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> SetOption(isize level, isize option, const u8* value,
                                    usize valueSize)
    {
        return Error(ENOSYS);
    }

  protected:
    SocketDomain    m_Domain = SocketDomain::eUnspecified;
    SocketType      m_Type   = SocketType::eRaw;
    NetworkProtocol m_Protocol;
    SocketState     m_State = SocketState::eUnconnected;
};
