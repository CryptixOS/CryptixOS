/*
 * Created by v1tr10l7 on 22.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <API/Posix/bits/socket.h>
#include <API/Posix/bits/socket_type.h>
#include <Prism/Containers/Queue.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
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
#include <linux/un.h>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

module Cryptix:Impl.Network.Socket;

import :Library.Logger;
import :Network.IPv4Socket;
import :Network.LocalSocket;
import :Network.NetLinkSocket;
import :Network.Socket;
import :Scheduler.Process;

Socket::Socket(SocketDomain domain, SocketType type, NetworkProtocol protocol)
    : m_Domain(domain)
    , m_Type(type)
    , m_Protocol(protocol)
{
}

ErrorOr<Socket*> Socket::Create(SocketDomain domain, SocketType type,
                                NetworkProtocol protocol)
{
    if (domain < SocketDomain::eUnspecified || domain >= SocketDomain::eCount)
        return Error(EAFNOSUPPORT);
    if (type < SocketType::eStream || type >= SocketType::eCount)
        return Error(EINVAL);

    // FIXME(v1tr10l7): Check if socket already exists

    switch (domain)
    {
        case SocketDomain::eLocal:
            return LocalSocket::Create(domain, type, protocol);
        case SocketDomain::eNetLink:
            return NetLinkSocket::Create(
                domain, type, static_cast<NetLinkProtocol>(protocol));

        default:
            LogError("Socket: Unsupported socket domain!");
            errno = ENOSYS;
            break;
    }

    return nullptr;
}
ErrorOr<::Ref<Socket>> Socket::Get(isize sockFdNum)
{
    auto                  process = Process::Current();
    ::Ref<FileDescriptor> sockFd
        = TryOrRet(process->GetFileDescriptor(sockFdNum));
    if (!sockFd)
        LogError("Socket::Get: No file descriptor found for fd number {:#x}",
                 sockFdNum);
    if (!sockFd->IsSocket())
        LogError(
            "Socket::Get: File descriptor at fd number {:#x} is not a socket!",
            sockFdNum);
    if (!sockFd->File())
        LogError(
            "Socket::Get: File descriptor at fd number {:#x} has no associated "
            "file!",
            sockFdNum);

    if (!sockFd || !sockFd->IsSocket() || !sockFd->File())
        return Error(ENOTSOCK);

    return reinterpret_cast<class Socket*>(sockFd->File());
}

ErrorOr<void> Socket::QueueConnectionFrom(Socket* peer)
{
    if (m_Pending.Size() >= m_BackLog) return Error(ECONNREFUSED);
    m_Pending.PushBack(peer);

    return {};
}
