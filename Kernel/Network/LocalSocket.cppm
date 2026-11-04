/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/bits/socket.h>
#include <API/Posix/bits/socket_type.h>
#include <Prism/Containers/Queue.hpp>
#include <Prism/Core/Types.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Deque.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <API/Posix/linux/un.h>

export module Cryptix:Network.LocalSocket;

export import :Network.Socket;

export class LocalSocket : public Socket
{
  public:
    LocalSocket(SocketDomain, SocketType type, NetworkProtocol proto);
    virtual ~LocalSocket();

    static ErrorOr<LocalSocket*> Create(SocketDomain domain, SocketType type,
                                        NetworkProtocol proto);

    virtual ErrorOr<void>        Connect(const sockaddr* addr,
                                         socklen_t              addrlen) override;
    virtual ErrorOr<void>  Accept(sockaddr* addr, socklen_t* addrlen) override;
    virtual ErrorOr<isize> SendTo(u8* data, usize size, isize flags) override;
    virtual ErrorOr<isize> ReceiveFrom(u8* data, usize size, isize flags,
                                       sockaddr*  saddr,
                                       socklen_t* addrlen) override;

    virtual ErrorOr<isize> SendMsg(const msghdr* msg,
                                   isize                flags) override;
    virtual ErrorOr<void>  Bind(const sockaddr* addr,
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
