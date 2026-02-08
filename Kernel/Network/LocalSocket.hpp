/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/Socket.hpp>
#include <linux/un.h>

class LocalSocket : public Socket
{
  public:
    LocalSocket(SocketDomain, SocketType type, NetworkProtocol proto);
    virtual ~LocalSocket();

    static ErrorOr<LocalSocket*> Create(SocketDomain domain, SocketType type,
                                        NetworkProtocol proto);

    virtual ErrorOr<void>        Connect(const struct sockaddr* addr,
                                         socklen_t              addrlen) override;
    virtual ErrorOr<void>  Accept(sockaddr* addr, socklen_t* addrlen) override;
    virtual ErrorOr<isize> SendTo(u8* data, usize size, isize flags) override;
    virtual ErrorOr<isize> ReceiveFrom(u8* data, usize size, isize flags,
                                       sockaddr*  saddr,
                                       socklen_t* addrlen) override;

    virtual ErrorOr<isize> SendMsg(const struct msghdr* msg,
                                   isize                flags) override;
    virtual ErrorOr<void>  Bind(const struct sockaddr* addr,
                                socklen_t              len) override;
    virtual ErrorOr<void>  Listen(i32 backlog) override;
    virtual ErrorOr<void>  GetLocalAddress(sockaddr*  addr,
                                           socklen_t* addrlen) override
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> GetPeerAddress(sockaddr*  addr,
                                         socklen_t* addrlen) override
    {
        return Error(ENOSYS);
    }

    virtual ErrorOr<void> GetOption(isize level, isize option, u8* out,
                                    socklen_t* outSize) override
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> SetOption(isize level, isize option, const u8* value,
                                    usize valueSize) override
    {
        return Error(ENOSYS);
    }

  private:
    class INode* m_INode = nullptr;
    String       m_Path;
    LocalSocket* m_Peer = nullptr;

    UserID       m_PreBindUID{0};
    GroupID      m_PreBindGID{0};
    INodeMode    m_PreBindMode{0};
};
