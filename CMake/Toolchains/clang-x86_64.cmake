#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Toolchain for building the kernel with clang, the CMake counterpart of
# CrossFiles/kernel-target-clang-x86_64.cross-file.
#
#   cmake -S . -B build -G Ninja --toolchain CMake/Toolchains/clang-x86_64.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_COMPILER_TARGET x86_64-linux-elf)
endforeach()

set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_NM llvm-nm)
set(CMAKE_STRIP llvm-strip)
set(CMAKE_OBJCOPY llvm-objcopy)
set(CMAKE_OBJDUMP llvm-objdump)
set(CMAKE_EXE_LINKER_FLAGS_INIT -fuse-ld=lld)

# There is no libc to link test programs against.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
