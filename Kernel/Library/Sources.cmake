#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    ExecutableProgram.cpp
    ICxxAbi.cpp
    Image.cpp
    KernelObject.cpp
    Logger.cpp
    Module.cpp
    Stacktrace.cpp
    ZLib.cpp
)

ctos_subdir(ELF)
ctos_subdir(KLibC)
ctos_subdir(KLibC++)
ctos_subdir(Locking)
