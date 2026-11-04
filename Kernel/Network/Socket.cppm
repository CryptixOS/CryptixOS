/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
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

export module Cryptix:Network.Socket;

export import :API.Sockets;
export import :VFS.File;

export enum class SocketState
{
    eNotConnected = 0,
    eConnected    = 1,
    eListening    = 2,
};
export class Socket : public File
{
  public:
    Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol);

    static ErrorOr<Socket*>       Create(SocketDomain domain, SocketType type,
                                         NetworkProtocol protocol);
    static ErrorOr<::Ref<Socket>> Get(isize sockFdNum);

    virtual ErrorOr<void>         Connect(const sockaddr* addr,
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
    virtual ErrorOr<isize> SendMsg(const msghdr* msg, isize flags)
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<void> Bind(const sockaddr* addr, socklen_t addrlen)
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

    virtual bool  IsSocket() const override { return true; }

    ErrorOr<void> QueueConnectionFrom(Socket* peer);

  protected:
    SocketDomain    m_Domain = SocketDomain::eUnspecified;
    SocketType      m_Type   = SocketType::eRaw;
    NetworkProtocol m_Protocol;
    SocketState     m_State = SocketState::eNotConnected;

    Deque<Socket*>  m_Pending;
    usize           m_BackLog = 0;
};
