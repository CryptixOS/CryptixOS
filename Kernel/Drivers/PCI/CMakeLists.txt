#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    Access.cppm
    Bus.cppm
    Definitions.cppm
    Device.cppm
    HostController.cppm
    MSI.cppm
    PCI.cppm
    Access.cpp
    Bus.cpp
    Device.cpp
    HostController.cpp
    PCI.cpp
)
