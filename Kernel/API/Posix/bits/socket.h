/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

/* Protocol families.  */
/* Unspecified.  */
constexpr usize PF_UNSPEC     = 0;
/* Local to host (pipes and file-domain).  */
constexpr usize PF_LOCAL      = 1;
/* POSIX name for PF_LOCAL.  */
constexpr usize PF_UNIX       = PF_LOCAL;
/* Another non-standard name for PF_LOCAL.  */
constexpr usize PF_FILE       = PF_LOCAL;
/* IP protocol family.  */
constexpr usize PF_INET       = 2;
/* Amateur Radio AX.25.  */
constexpr usize PF_AX25       = 3;
/* Novell Internet Protocol.  */
constexpr usize PF_IPX        = 4;
/* Appletalk DDP.  */
constexpr usize PF_APPLETALK  = 5;
/* Amateur radio NetROM.  */
constexpr usize PF_NETROM     = 6;
/* Multiprotocol bridge.  */
constexpr usize PF_BRIDGE     = 7;
/* ATM PVCs.  */
constexpr usize PF_ATMPVC     = 8;
/* Reserved for X.25 project.  */
constexpr usize PF_X25        = 9;
/* IP version 6.  */
constexpr usize PF_INET6      = 10;
/* Amateur Radio X.25 PLP.  */
constexpr usize PF_ROSE       = 11;
/* Reserved for DECnet project.  */
constexpr usize PF_DECnet     = 12;
/* Reserved for 802.2LLC project.  */
constexpr usize PF_NETBEUI    = 13;
/* Security callback pseudo AF.  */
constexpr usize PF_SECURITY   = 14;
/* PF_KEY key management API.  */
constexpr usize PF_KEY        = 15;
constexpr usize PF_NETLINK    = 16;
/* Alias to emulate 4.4BSD.  */
constexpr usize PF_ROUTE      = PF_NETLINK;
/* Packet family.  */
constexpr usize PF_PACKET     = 17;
/* Ash.  */
constexpr usize PF_ASH        = 18;
/* Acorn Econet.  */
constexpr usize PF_ECONET     = 19;
/* ATM SVCs.  */
constexpr usize PF_ATMSVC     = 20;
/* RDS sockets.  */
constexpr usize PF_RDS        = 21;
/* Linux SNA Project */
constexpr usize PF_SNA        = 22;
/* IRDA sockets.  */
constexpr usize PF_IRDA       = 23;
/* PPPoX sockets.  */
constexpr usize PF_PPPOX      = 24;
/* Wanpipe API sockets.  */
constexpr usize PF_WANPIPE    = 25;
/* Linux LLC.  */
constexpr usize PF_LLC        = 26;
/* Native InfiniBand address.  */
constexpr usize PF_IB         = 27;
/* MPLS.  */
constexpr usize PF_MPLS       = 28;
/* Controller Area Network.  */
constexpr usize PF_CAN        = 29;
/* TIPC sockets.  */
constexpr usize PF_TIPC       = 30;
/* Bluetooth sockets.  */
constexpr usize PF_BLUETOOTH  = 31;
/* IUCV sockets.  */
constexpr usize PF_IUCV       = 32;
/* RxRPC sockets.  */
constexpr usize PF_RXRPC      = 33;
/* mISDN sockets.  */
constexpr usize PF_ISDN       = 34;
/* Phonet sockets.  */
constexpr usize PF_PHONET     = 35;
/* IEEE 802.15.4 sockets.  */
constexpr usize PF_IEEE802154 = 36;
/* CAIF sockets.  */
constexpr usize PF_CAIF       = 37;
/* Algorithm sockets.  */
constexpr usize PF_ALG        = 38;
/* NFC sockets.  */
constexpr usize PF_NFC        = 39;
/* vSockets.  */
constexpr usize PF_VSOCK      = 40;
/* Kernel Connection Multiplexor.  */
constexpr usize PF_KCM        = 41;
/* Qualcomm IPC Router.  */
constexpr usize PF_QIPCRTR    = 42;
/* SMC sockets.  */
constexpr usize PF_SMC        = 43;
/* XDP sockets.  */
constexpr usize PF_XDP        = 44;
/* Management component transport protocol.  */
constexpr usize PF_MCTP       = 45;
/* For now..  */
constexpr usize PF_MAX        = 46;

/* Address families.  */
constexpr usize AF_UNSPEC     = PF_UNSPEC;
constexpr usize AF_LOCAL      = PF_LOCAL;
constexpr usize AF_UNIX       = PF_UNIX;
constexpr usize AF_FILE       = PF_FILE;
constexpr usize AF_INET       = PF_INET;
constexpr usize AF_AX25       = PF_AX25;
constexpr usize AF_IPX        = PF_IPX;
constexpr usize AF_APPLETALK  = PF_APPLETALK;
constexpr usize AF_NETROM     = PF_NETROM;
constexpr usize AF_BRIDGE     = PF_BRIDGE;
constexpr usize AF_ATMPVC     = PF_ATMPVC;
constexpr usize AF_X25        = PF_X25;
constexpr usize AF_INET6      = PF_INET6;
constexpr usize AF_ROSE       = PF_ROSE;
constexpr usize AF_DECnet     = PF_DECnet;
constexpr usize AF_NETBEUI    = PF_NETBEUI;
constexpr usize AF_SECURITY   = PF_SECURITY;
constexpr usize AF_KEY        = PF_KEY;
constexpr usize AF_NETLINK    = PF_NETLINK;
constexpr usize AF_ROUTE      = PF_ROUTE;
constexpr usize AF_PACKET     = PF_PACKET;
constexpr usize AF_ASH        = PF_ASH;
constexpr usize AF_ECONET     = PF_ECONET;
constexpr usize AF_ATMSVC     = PF_ATMSVC;
constexpr usize AF_RDS        = PF_RDS;
constexpr usize AF_SNA        = PF_SNA;
constexpr usize AF_IRDA       = PF_IRDA;
constexpr usize AF_PPPOX      = PF_PPPOX;
constexpr usize AF_WANPIPE    = PF_WANPIPE;
constexpr usize AF_LLC        = PF_LLC;
constexpr usize AF_IB         = PF_IB;
constexpr usize AF_MPLS       = PF_MPLS;
constexpr usize AF_CAN        = PF_CAN;
constexpr usize AF_TIPC       = PF_TIPC;
constexpr usize AF_BLUETOOTH  = PF_BLUETOOTH;
constexpr usize AF_IUCV       = PF_IUCV;
constexpr usize AF_RXRPC      = PF_RXRPC;
constexpr usize AF_ISDN       = PF_ISDN;
constexpr usize AF_PHONET     = PF_PHONET;
constexpr usize AF_IEEE802154 = PF_IEEE802154;
constexpr usize AF_CAIF       = PF_CAIF;
constexpr usize AF_ALG        = PF_ALG;
constexpr usize AF_NFC        = PF_NFC;
constexpr usize AF_VSOCK      = PF_VSOCK;
constexpr usize AF_KCM        = PF_KCM;
constexpr usize AF_QIPCRTR    = PF_QIPCRTR;
constexpr usize AF_SMC        = PF_SMC;
constexpr usize AF_XDP        = PF_XDP;
constexpr usize AF_MCTP       = PF_MCTP;
constexpr usize AF_MAX        = PF_MAX;

/* Socket level values.  Others are defined in the appropriate headers.

   XXX These definitions also should go into the appropriate headers as
   far as they are available.  */
constexpr usize SOL_RAW       = 255;
constexpr usize SOL_DECNET    = 261;
constexpr usize SOL_X25       = 262;
constexpr usize SOL_PACKET    = 263;
/* ATM layer (cell level).  */
constexpr usize SOL_ATM       = 264;
/* ATM Adaption Layer (packet level).  */
constexpr usize SOL_AAL       = 265;
constexpr usize SOL_IRDA      = 266;
constexpr usize SOL_NETBEUI   = 267;
constexpr usize SOL_LLC       = 268;
constexpr usize SOL_DCCP      = 269;
constexpr usize SOL_NETLINK   = 270;
constexpr usize SOL_TIPC      = 271;
constexpr usize SOL_RXRPC     = 272;
constexpr usize SOL_PPPOL2TP  = 273;
constexpr usize SOL_BLUETOOTH = 274;
constexpr usize SOL_PNPIPE    = 275;
constexpr usize SOL_RDS       = 276;
constexpr usize SOL_IUCV      = 277;
constexpr usize SOL_CAIF      = 278;
constexpr usize SOL_ALG       = 279;
constexpr usize SOL_NFC       = 280;
constexpr usize SOL_KCM       = 281;
constexpr usize SOL_TLS       = 282;
constexpr usize SOL_XDP       = 283;
constexpr usize SOL_MPTCP     = 284;
constexpr usize SOL_MCTP      = 285;
constexpr usize SOL_SMC       = 286;
constexpr usize SOL_VSOCK     = 287;

/* Maximum queue length specifiable by listen.  */
constexpr usize SOMAXCONN     = 4096;

using socklen_t               = u32;
using sa_family_t             = unsigned short int;

struct sockaddr
{
    sa_family_t sa_family;
    u8          sa_data[14];
};
