#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    FullDevice.cppm
    InterruptController.cppm
    KernelMessageDevice.cppm
    NullDevice.cppm
    RandomDevice.cppm
    Serial.cppm
    ZeroDevice.cppm
    KernelMessageDevice.cpp
)

add_subdirectory(ACPI)
add_subdirectory(Core)
add_subdirectory(Input)
add_subdirectory(PCI)
add_subdirectory(Storage)
add_subdirectory(TTY)
add_subdirectory(USB)
add_subdirectory(Video)
add_subdirectory(Virtio)
