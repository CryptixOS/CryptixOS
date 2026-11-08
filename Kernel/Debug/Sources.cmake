#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    Kasan.cpp
    Panic.cpp
)

set(CTOS_MM_DUMP_MEMMAP 0)
set(CTOS_PCI_DUMP_IRQ_ROUTES 0)
set(CTOS_DTB_DUMP 0)
set(CTOS_EFI_DUMP_MEMMAP 0)
set(CTOS_TTY_LOG_TERMIOS 0)
set(CTOS_DUMP_INIT_ARRAY 1)
set(CTOS_ELF_DEBUG 0)
set(CTOS_DEVICE_TREE_DUMP 0)
ctos_configure_meson_template(${CMAKE_CURRENT_LIST_DIR}/Debug.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/Debug/Debug.hpp)
