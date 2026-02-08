/*
 * Created by v1tr10l7 on 05.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/LocalSocket.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Scheduler/Process.hpp>
#include <VFS/VFS.hpp>

static UnorderedMap<INode*, LocalSocket*> s_Sockets;

LocalSocket::LocalSocket(SocketDomain, SocketType type, NetworkProtocol proto)
    : Socket(SocketDomain::eLocal, type, proto)
{
    auto process  = Process::Current();
    m_PreBindUID  = process->UserID();
    m_PreBindGID  = process->GroupID();
    m_PreBindMode = 0666;
}
LocalSocket::~LocalSocket() {}

ErrorOr<LocalSocket*> LocalSocket::Create(SocketDomain domain, SocketType type,
                                          NetworkProtocol proto)
{
    Assert(domain == SocketDomain::eLocal);

    return new LocalSocket(domain, type, proto);
}

ErrorOr<void> LocalSocket::Connect(const struct sockaddr* addr,
                                   socklen_t              addrlen)
{
    Assert(m_State == SocketState::eNotConnected);
    if (addrlen != sizeof(sockaddr_un))
    {
        LogWarn("LocalSocket::Connect: Invalid sockaddr_un length: {}",
                addrlen);
        return Error(EINVAL);
    }

    sockaddr_un address = {};
    CopyFromUser(&address, addr, addrlen);
    if (address.sun_family != AF_LOCAL)
    {
        LogWarn("LocalSocket::Connect: Invalid address family: {}",
                address.sun_family);
        return Error(EINVAL);
    }

    auto path = address.sun_path;

    LogTrace("LocalSocket: Connecting to socket at `{}`", path);
    auto pathRes  = TryOrRet(VFS::ResolvePath(VFS::RootDirectoryEntry(), path));
    auto entry    = pathRes.Entry;
    auto inode    = entry->INode();

    auto socketIt = s_Sockets.Find(inode.Raw());
    if (socketIt == s_Sockets.end()) return Error(ECONNREFUSED);

    auto listener = socketIt->Value;
    if (listener->m_State != SocketState::eListening)
        return Error(ECONNREFUSED);

    auto peer = TryOrRet(
        LocalSocket::Create(SocketDomain::eLocal, m_Type, m_Protocol));
    m_Peer       = peer;
    peer->m_Peer = this;

    m_State      = SocketState::eConnected;

    auto result  = listener->QueueConnectionFrom(peer);
    if (!result) return Error(result.Error());

    return {};
}
ErrorOr<void> LocalSocket::Accept(sockaddr* addr, socklen_t* addrlen)
{
    return Error(ENOSYS);
}
ErrorOr<isize> LocalSocket::SendTo(u8* data, usize size, isize flags)
{
    return Error(ENOSYS);
}
ErrorOr<isize> LocalSocket::ReceiveFrom(u8* data, usize size, isize flags,
                                        sockaddr* saddr, socklen_t* addrlen)
{
    return Error(ENOSYS);
}

ErrorOr<isize> LocalSocket::SendMsg(const struct msghdr* msg, isize flags)
{
    LogDebug("LocalSocket::SendMsg called");
    return Error(ENOSYS);
}
ErrorOr<void> LocalSocket::Bind(const struct sockaddr* addr, socklen_t len)
{
    Assert(m_State == SocketState::eNotConnected);
    if (len != sizeof(sockaddr_un))
    {
        LogWarn("LocalSocket::Bind: Invalid sockaddr_un length: {}", len);
        return Error(EINVAL);
    }

    sockaddr_un address = {};
    CopyFromUser(&address, addr, len);

    if (address.sun_family != AF_LOCAL)
    {
        LogWarn("LocalSocket::Bind: Invalid address family: {}",
                address.sun_family);
        return Error(EINVAL);
    }

    auto      path   = address.sun_path;
    INodeMode mode   = S_IFSOCK | (m_PreBindMode & 0777);

    auto      result = VFS::ResolvePath(VFS::RootDirectoryEntry(), path);
    ::Ref<DirectoryEntry> dentry = result ? result->Entry : nullptr;

    if (!dentry)
    {
        LogTrace("LocalSocket::Bind: Creating socket file at path: {}", path);
        auto result = VFS::CreateNode(path, mode, 0);

        if (!result)
        {
            LogWarn(
                "LocalSocket::Bind: Failed to create socket file at path: {}, "
                "error: {}",
                path, StringUtils::ToString(result.Error()));
            if (result.Error() == EEXIST) return Error(EADDRINUSE);
            return Error(result.Error());
        }

        dentry = *result;
    }

    // auto      result = VFS::Open(VFS::RootDirectoryEntry(), path,
    //                              O_CREAT | O_EXCL | O_NOFOLLOW, mode);

    auto inode = dentry->INode();

    if (s_Sockets.Contains(inode.Raw())) return Error(EADDRINUSE);
    s_Sockets[inode.Raw()] = this;
    m_INode                = inode.Raw();

    m_Path                 = Move(path);
    return {};
}
ErrorOr<void> LocalSocket::Listen(i32 backlog)
{
    if (m_Type != SocketType::eStream
        && m_Type != SocketType::eSequentialPacket)
        return Error(EOPNOTSUPP);
    if (!m_INode) return Error(EINVAL);

    if (m_State != SocketState::eNotConnected
        && m_State != SocketState::eListening)
        return Error(EINVAL);

    m_BackLog = static_cast<usize>(backlog);
    m_State   = SocketState::eListening;

    // TODO(v1tr10l7): save pid and creds
    return {};
}
