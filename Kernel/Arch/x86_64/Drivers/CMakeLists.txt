#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    IoApic.cppm
    PCSpeaker.cppm
    PIC.cppm
    IoApic.cpp
    PCI.cpp
    PCSpeaker.cpp
    PIC.cpp
    Serial.cpp
)

add_subdirectory(Time)
