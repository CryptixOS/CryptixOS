/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

/* Types of sockets.  */
enum __socket_type
{
    SOCK_STREAM = 1, /* Sequenced, reliable, connection-based
                        byte streams.  */
#define SOCK_STREAM SOCK_STREAM
    SOCK_DGRAM = 2, /* Connectionless, unreliable datagrams
                       of fixed maximum length.  */
#define SOCK_DGRAM SOCK_DGRAM
    SOCK_RAW = 3, /* Raw protocol interface.  */
#define SOCK_RAW SOCK_RAW
    SOCK_RDM = 4, /* Reliably-delivered messages.  */
#define SOCK_RDM SOCK_RDM
    SOCK_SEQPACKET = 5, /* Sequenced, reliable, connection-based,
                           datagrams of fixed maximum length.  */
#define SOCK_SEQPACKET SOCK_SEQPACKET
    SOCK_DCCP = 6, /* Datagram Congestion Control Protocol.  */
#define SOCK_DCCP SOCK_DCCP
    SOCK_PACKET = 10, /* Linux specific way of getting packets
                         at the dev level.  For writing rarp and
                         other similar things on the user level. */
#define SOCK_PACKET SOCK_PACKET

    /* Flags to be ORed into the type parameter of socket and socketpair and
       used for the flags parameter of paccept.  */

    SOCK_CLOEXEC = 02000000, /* Atomically set close-on-exec flag for the
                                new descriptor(s).  */
#define SOCK_CLOEXEC SOCK_CLOEXEC
    SOCK_NONBLOCK = 00004000 /* Atomically mark descriptor(s) as
                                non-blocking.  */
#define SOCK_NONBLOCK SOCK_NONBLOCK
};
