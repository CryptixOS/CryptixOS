/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/linux/netlink.h>
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

module Cryptix:Impl.Network.NetLinkSocket;

import :Network.NetLinkSocket;
import :Scheduler.Process;

static UnorderedMap<NetLinkProtocol, NetLinkProtocolInfo> s_NetLinkProtocols;

NetLinkSocket::NetLinkSocket(SocketDomain domain, SocketType type,
                             NetLinkProtocol protocol)
    : Socket(domain, type, static_cast<NetworkProtocol>(protocol))
    , m_NetLinkProtocol(protocol)
{
}

ErrorOr<NetLinkSocket*> NetLinkSocket::Create(SocketDomain    domain,
                                              SocketType      type,
                                              NetLinkProtocol protocol)
{
    if (type != SocketType::eRaw && type != SocketType::eDataGram)
        return Error(ESOCKTNOSUPPORT);

    if (protocol < NetLinkProtocol::eRoute || protocol >= NetLinkProtocol::eMax)
        return Error(EPROTONOSUPPORT);

    return new NetLinkSocket(domain, type, protocol);
}

ErrorOr<void> NetLinkSocket::Bind(const sockaddr* addr, socklen_t len)
{
    auto        process = Process::Current();
    sockaddr_nl nladdr
        = CopyFromUser(*reinterpret_cast<const sockaddr_nl*>(addr));
    if (nladdr.nl_family != AF_NETLINK) return Error(EINVAL);

    if (nladdr.nl_groups)
    {
        if (!process->IsSuperUser()) return Error(EPERM);
        if (!s_NetLinkProtocols[m_NetLinkProtocol].Registered)
            return Error(ENOENT);

        if (m_Groups.Size() >= s_NetLinkProtocols[m_NetLinkProtocol].Groups)
            return {};
        m_Groups.Resize(s_NetLinkProtocols[m_NetLinkProtocol].Groups);
        // realloc groups
    }

    if (m_Pid && nladdr.nl_pid != m_Pid) return Error(EINVAL);
    else if (!m_Pid)
    {
        // Insert socket
        // autobind
    }

    if (!nladdr.nl_groups && (m_Groups.Empty() || !m_Groups[0])) return {};

    return Error(ENOSYS);
}
