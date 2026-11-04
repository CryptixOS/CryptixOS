#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    BlockDevice.cppm
    CharacterDevice.cppm
    Device.cppm
    DeviceManager.cppm
    FilesystemDriver.cppm
    GenericDriver.cppm
    CharacterDevice.cpp
    Device.cpp
    DeviceManager.cpp
    GenericDriver.cpp
)
