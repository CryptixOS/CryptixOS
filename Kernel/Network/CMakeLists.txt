#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    IPv4Socket.cppm
    LocalSocket.cppm
    MacAddress.cppm
    NetLinkSocket.cppm
    NetworkAdapter.cppm
    Socket.cppm
    LocalSocket.cpp
    NetLinkSocket.cpp
    Socket.cpp
)
