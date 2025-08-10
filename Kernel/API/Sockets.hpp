/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/bits/socket.h>
#include <API/Posix/bits/socket_type.h>

enum class SocketDomain : i64
{
    eUnspecified = 0,
    eLocal       = AF_LOCAL,
    eUnix        = AF_UNIX,
    eINet        = AF_INET,
    eINet6       = AF_INET6,
    eNetLink     = AF_NETLINK,

    eCount,
};
enum class SocketType : i64
{
    eStream           = SOCK_STREAM,
    eDataGram         = SOCK_DGRAM,
    eSequentialPacket = SOCK_SEQPACKET,
    eRaw              = SOCK_RAW,
    eRDM              = SOCK_RDM,
    eNonBlock         = SOCK_NONBLOCK,
    eCloseOnExec      = SOCK_CLOEXEC,

    eCount,
};

enum class NetworkProtocol : i64
{
    eNone = 0,

    eCount,
};

enum class NetLinkProtocol : i64
{
    eRoute         = 0x00,
    eUnused        = 0x01,
    eUserSock      = 0x02,
    eFireWall      = 0x03,
    eSockDiag      = 0x04,
    eNetFilterLog  = 0x05,
    eXFRM          = 0x06,
    eSELinux       = 0x07,
    eISCSI         = 0x08,
    eAudit         = 0x09,
    eFibLookup     = 0x10,
    eConnector     = 0x11,
    eNetFilter     = 0x12,
    eIPv6Fw        = 0x13,
    eDECnetRouting = 0x14,
    eKObjectUEvent = 0x15,
    eGeneric       = 0x16,
    eSCSITransport = 0x18,
    eECryptFs      = 0x19,
    eRDMA          = 0x20,
    eCrypto        = 0x21,
    eSMC           = 0x22,
    eINetDiag      = eSockDiag,

    eMax           = 32,

};
