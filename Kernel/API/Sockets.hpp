/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/bits/socket.h>

enum class SocketDomain : i64
{
    eLocal   = AF_LOCAL,
    eUnix    = AF_UNIX,
    eINet    = AF_INET,
    eINet6   = AF_INET6,
    eNetLink = AF_NETLINK
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
};
