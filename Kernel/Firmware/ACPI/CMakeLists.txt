#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    ACPI.cppm
    HPET.cppm
    MADT.cppm
    MCFG.cppm
    SRAT.cppm
    Structures.cppm
    Tables.cppm
    ACPI.cpp
    MADT.cpp
    UACPI.cpp
)

add_subdirectory(Interpreter)
