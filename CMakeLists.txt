#
# Created by v1tr10l7 on 16.11.2024.
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#
# The kernel is written as a single C++20 named module, `Cryptix`, whose interface
# partitions replace the former headers.  Building it needs:
#   * CMake >= 3.28 with the Ninja generator,
#   * clang >= 17 plus `clang-scan-deps` (module dependency scanning),
#   * lld, llvm-ar, nasm-free assembly via clang.
#
cmake_minimum_required(VERSION 3.28)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR
    "Select a target with a preset (cmake --preset x86_64-release) or pass\n"
    "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-<arch>.cmake")
endif()

project(CryptixOS VERSION 0.0.1 LANGUAGES C CXX ASM)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(FATAL_ERROR "C++20 modules need the Ninja generator (cmake -G Ninja).")
endif()
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "CryptixOS needs clang 17 or newer (found ${CMAKE_CXX_COMPILER_ID}).")
endif()
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
  message(FATAL_ERROR "CryptixOS needs clang 17 or newer (found ${CMAKE_CXX_COMPILER_VERSION}).")
endif()

# ------------------------------------------------------------------------------
# Target architecture
# ------------------------------------------------------------------------------
set(CRYPTIX_ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "Target architecture")
if(CRYPTIX_ARCH STREQUAL "x86_64")
  set(CRYPTIX_FREESTND_ARCH x86_64)
  set(CRYPTIX_ARCH_ID    CTOS_ARCH_X86_64)
  set(CRYPTIX_ARCH_DEFS  CTOS_TARGET_X86_64)
elseif(CRYPTIX_ARCH STREQUAL "aarch64")
  set(CRYPTIX_FREESTND_ARCH aarch64)
  set(CRYPTIX_ARCH_ID    CTOS_ARCH_AARCH64)
  set(CRYPTIX_ARCH_DEFS  CTOS_TARGET_AARCH64)
else()
  message(FATAL_ERROR "Unknown architecture: ${CRYPTIX_ARCH}")
endif()

option(CRYPTIX_WERROR "Treat warnings as errors" ON)

set(CMAKE_C_STANDARD   17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)    # we pass -fPIE ourselves
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Debug / release / dist  <->  meson's debug / debugoptimized / release
set(CMAKE_C_FLAGS_DEBUG            "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_C_FLAGS_RELEASE          "-O3")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ------------------------------------------------------------------------------
# Compiler flags shared by every freestanding target
# ------------------------------------------------------------------------------
set(CRYPTIX_DEFINITIONS
  FMT_HEADER_ONLY
  FMT_STATIC_THOUSANDS_SEPARATOR=1
  FMT_USE_LONG_DOUBLE=0
  FMT_USE_DOUBLE=0
  FMT_USE_FLOAT=0
  FMT_USE_FLOAT128=0
  FMT_EXCEPTIONS=0
  LIMINE_API_REVISION=3
  MAGIC_ENUM_NO_STREAMS=1
  MAGIC_ENUM_RANGE_MAX=0x8a00
  PRISM_BUILD_DEBUG=1
  PRISM_LOG_ENABLE=1
  PRISM_TARGET_CRYPTIX_KERNEL=1
  PRISM_TARGET_CRYPTIX=1
  PRISM_USE_NAMESPACE=1
  PRISM_BUILD_CRYPTIX_KERNEL=1
  UACPI_FORMATTED_LOGGING
  FMT_BUILTIN_TYPES=0
  _LIBCPP_HAS_NO_THREADS
  CTOS_ARCH=${CRYPTIX_ARCH_ID}
  ${CRYPTIX_ARCH_DEFS})

set(CRYPTIX_COMMON_FLAGS
  -ffreestanding
  -fno-omit-frame-pointer           # keep the frame pointer for stack traces
  -fno-lto
  -fsanitize=undefined
  -fstack-protector-all
  -mstack-protector-guard=global
  -Wimplicit-fallthrough
  -Wno-builtin-macro-redefined
  -Wno-macro-redefined
  -Wno-nan-infinity-disabled
  -Wno-deprecated-builtins
  -Wno-unused-parameter
  -Wno-frame-address
  -Wno-c23-extensions)

add_library(cryptix_c_flags INTERFACE)
target_compile_definitions(cryptix_c_flags INTERFACE ${CRYPTIX_DEFINITIONS})
target_compile_options(cryptix_c_flags INTERFACE ${CRYPTIX_COMMON_FLAGS})

add_library(cryptix_cxx_flags INTERFACE)
target_link_libraries(cryptix_cxx_flags INTERFACE cryptix_c_flags)
target_compile_options(cryptix_cxx_flags INTERFACE
  $<$<COMPILE_LANGUAGE:CXX>:-fsized-deallocation>     # pass the size to operator delete
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-non-virtual-dtor>
  $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)

# The kernel and the loadable modules import the same module interface (BMI), so the
# target feature flags have to be identical; only the code model differs.
if(CRYPTIX_ARCH STREQUAL "x86_64")
  set(CRYPTIX_ARCH_FLAGS -march=x86-64 -m64 -msoft-float -mno-mmx -mno-sse -mno-sse2
                         -mno-avx -mno-red-zone -mrdrnd -mrdseed)
  set(CRYPTIX_KERNEL_ARCH_FLAGS ${CRYPTIX_ARCH_FLAGS} -mcmodel=kernel)
  set(CRYPTIX_MODULE_ARCH_FLAGS ${CRYPTIX_ARCH_FLAGS} -mcmodel=large)
else()
  set(CRYPTIX_ARCH_FLAGS -mgeneral-regs-only -march=armv8-a+nofp+nosimd)
  set(CRYPTIX_KERNEL_ARCH_FLAGS ${CRYPTIX_ARCH_FLAGS} -mcmodel=small)
  set(CRYPTIX_MODULE_ARCH_FLAGS ${CRYPTIX_ARCH_FLAGS} -mcmodel=small)
endif()

# Code model + CPU flags of everything that is linked into the kernel image (a static PIE).
# Loadable modules are position independent instead (-fPIC), see Modules/CMakeLists.txt.
add_library(cryptix_kernel_arch INTERFACE)
target_compile_options(cryptix_kernel_arch INTERFACE -fno-PIC -fPIE ${CRYPTIX_KERNEL_ARCH_FLAGS})

# Include directories shared by the kernel, Prism and the bundled C/C++ libraries.
add_library(cryptix_includes INTERFACE)
target_include_directories(cryptix_includes INTERFACE
  ${PROJECT_SOURCE_DIR}/Kernel
  ${PROJECT_SOURCE_DIR}/Kernel/Library/KLibC
  ${PROJECT_SOURCE_DIR}/Kernel/Library/KLibC++
  ${PROJECT_SOURCE_DIR}/Prism/Source)

# Assembly goes through the C preprocessor with the same flags.
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding")

include(Dependencies)

# ------------------------------------------------------------------------------
# Prism (header based standard library, a git submodule) and the kernel
# ------------------------------------------------------------------------------
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/Prism/Source/Prism)
  message(FATAL_ERROR "The Prism submodule is missing: run `git submodule update --init`.")
endif()
add_library(prism OBJECT
  Prism/Source/Prism/Algorithm/Hash.cpp
  Prism/Source/Prism/Containers/Bitmap.cpp
  Prism/Source/Prism/Containers/RingBuffer.cpp
  Prism/Source/Prism/Debug/Assertions.cpp
  Prism/Source/Prism/Debug/Log.cpp
  Prism/Source/Prism/Debug/Logger.cpp
  Prism/Source/Prism/Debug/Ubsan.cpp
  Prism/Source/Prism/Debug/Stacktrace.cpp
  Prism/Source/Prism/Memory/Memory.cpp
  Prism/Source/Prism/String/Printf.cpp
  Prism/Source/Prism/Utility/Path.cpp
  Prism/Source/Prism/Utility/PathView.cpp)
target_link_libraries(prism PUBLIC cryptix_cxx_flags cryptix_includes cryptix_headers PRIVATE cryptix_kernel_arch)
set_target_properties(prism PROPERTIES CXX_SCAN_FOR_MODULES OFF)

add_subdirectory(Kernel)
add_subdirectory(Modules)

include(Image)
