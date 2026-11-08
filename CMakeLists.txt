#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# CMake build of the kernel and its loadable modules. It builds the same
# binaries as meson.build, which is kept until this build replaces it.
# CMake only compiles and links: Cryptix.elf and the .ko modules. The
# initrd, disk image, ISO and QEMU runs are done by Meta/run.py.
#
#   cmake -S . -B build_cmake -G Ninja \
#         --toolchain CMake/Toolchains/clang-x86_64.cmake
#   cmake --build build_cmake
cmake_minimum_required(VERSION 3.30)

project(CryptixOS
    VERSION 0.0.1
    LANGUAGES C CXX ASM
)

set(CTOS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
if(NOT CTOS_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "Unknown architecture: '${CTOS_ARCH}', "
        "configure with --toolchain CMake/Toolchains/<compiler>-<arch>.cmake")
endif()

# ==============================================================================
### Build types, matching meson's: Debug = debug,
### RelWithDebInfo = debugoptimized, Release = release. NDEBUG is never set,
### meson's b_ndebug is false.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()
foreach(lang C CXX ASM)
    set(CMAKE_${lang}_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(CMAKE_${lang}_FLAGS_RELEASE "-O3")
endforeach()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CTOS_USE_CCACHE "Compile through ccache" ON)
option(CTOS_KASAN "Build the kernel with KASAN (Debug/Kasan.hpp), clang only" OFF)
find_program(CCACHE_PROGRAM ccache)
if(CTOS_USE_CCACHE AND CCACHE_PROGRAM)
    foreach(lang C CXX ASM)
        set(CMAKE_${lang}_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    endforeach()
endif()

# ==============================================================================
### Compiler arguments, applied to everything, like add_global_arguments()
include(CheckCXXCompilerFlag)

set(CTOS_WARNING_ARGS
    -Wimplicit-fallthrough
    -Wno-builtin-macro-redefined
    -Wno-macro-redefined        # clang
    -Wno-nan-infinity-disabled  # clang
    -Wno-deprecated-builtins    # clang
    -Wno-unused-parameter
    -Wno-frame-address
    -Wno-c23-extensions         # clang
)
set(CTOS_MACRO_ARGS
    -DFMT_HEADER_ONLY
    -DFMT_STATIC_THOUSANDS_SEPARATOR=1
    -DFMT_USE_LONG_DOUBLE=0
    -DFMT_USE_DOUBLE=0
    -DFMT_USE_FLOAT=0
    -DFMT_USE_FLOAT128=0
    -DFMT_EXCEPTIONS=0
    -DLIMINE_API_REVISION=3
    -DMAGIC_ENUM_NO_STREAMS=1
    -DMAGIC_ENUM_RANGE_MAX=0x8a00
    -DPRISM_BUILD_DEBUG=1
    -DPRISM_LOG_ENABLE=1
    -DPRISM_TARGET_CRYPTIX_KERNEL=1
    -DPRISM_TARGET_CRYPTIX=1
    -DPRISM_USE_NAMESPACE=1
    -DPRISM_BUILD_CRYPTIX_KERNEL=1
    -DUACPI_FORMATTED_LOGGING
    -DFMT_BUILTIN_TYPES=0
    -D_LIBCPP_HAS_NO_THREADS
)
set(CTOS_GLOBAL_ARGS
    -ffreestanding
    # keep stack pointer in the appropriate register
    -fno-omit-frame-pointer
    -fno-lto
    -fsanitize=undefined
    -fstack-protector-all
    -mstack-protector-guard=global
    -fno-PIC
    -fPIE
    -nostdlib
    -static
)
foreach(arg ${CTOS_WARNING_ARGS})
    string(MAKE_C_IDENTIFIER "CTOS_HAS${arg}" var)
    check_cxx_compiler_flag(${arg} ${var})
    if(${var})
        list(APPEND CTOS_GLOBAL_ARGS ${arg})
    endif()
endforeach()
list(APPEND CTOS_GLOBAL_ARGS ${CTOS_MACRO_ARGS})

set(CTOS_GLOBAL_CXX_ARGS
    -D_GLIBCXX_ASSERTIONS=1
    -fno-exceptions
    -fno-rtti
    # pass the size argument to delete operator
    -fsized-deallocation
    -MP
    -Wno-non-virtual-dtor
    -nostdinc++
)
check_cxx_compiler_flag(-Wno-changes-meaning CTOS_HAS_WNO_CHANGES_MEANING)
if(CTOS_HAS_WNO_CHANGES_MEANING)
    list(APPEND CTOS_GLOBAL_CXX_ARGS -Wno-changes-meaning) # gcc
endif()

# meson's warning_level=2 and werror=true
add_compile_options(
    -Wall -Wextra -Werror
    -D_FILE_OFFSET_BITS=64
    # CMake passes absolute source paths. Make __FILE__ (assertions, logs)
    # relative like meson's, so file names stay short. Debug info keeps the
    # absolute paths.
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=../
    ${CTOS_GLOBAL_ARGS}
    "$<$<COMPILE_LANGUAGE:CXX>:${CTOS_GLOBAL_CXX_ARGS}>"
)

# ==============================================================================
### Dependencies
include(CMake/Dependencies.cmake)
include(CMake/Sources.cmake)

add_subdirectory(Modules)
add_subdirectory(Kernel)
