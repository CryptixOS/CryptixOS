#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PRIVATE
    PrismHooks.cpp
  PUBLIC FILE_SET CXX_MODULES FILES
    Color.cppm
    ExecutableProgram.cppm
    ICxxAbi.cppm
    Image.cppm
    KernelObject.cppm
    KernelSet.cppm
    Logger.cppm
    Module.cppm
    StackBuilder.cppm
    Stacktrace.cppm
    Unicode.cppm
    UserBuffer.cppm
    ZLib.cppm
    ExecutableProgram.cpp
    ICxxAbi.cpp
    Image.cpp
    KernelObject.cpp
    Logger.cpp
    Module.cpp
    Stacktrace.cpp
    ZLib.cpp
)

add_subdirectory(ELF)
add_subdirectory(KLibC)
add_subdirectory(Locking)
