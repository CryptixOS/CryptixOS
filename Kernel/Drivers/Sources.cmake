#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    KernelMessageDevice.cpp
)

ctos_subdir(ACPI)
ctos_subdir(Core)
ctos_subdir(Input)
ctos_subdir(PCI)
ctos_subdir(Storage)
ctos_subdir(USB)
ctos_subdir(TTY)
ctos_subdir(Video)
