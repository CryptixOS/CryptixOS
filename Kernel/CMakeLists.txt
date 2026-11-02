#
# Created by v1tr10l7 on 16.11.2024.
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#
# The kernel is one C++20 module, `Cryptix`.  Every directory below contributes its
# interface partitions (*.cppm) through the CXX_MODULES file set and its
# implementation units (*.cpp) as ordinary sources.
#

add_library(cryptix_kernel OBJECT)
target_link_libraries(cryptix_kernel PUBLIC cryptix_cxx_flags cryptix_includes cryptix_headers
                                           PRIVATE cryptix_kernel_arch)
target_include_directories(cryptix_kernel PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR})
target_compile_options(cryptix_kernel PRIVATE
  -Wall -Wextra
  $<$<BOOL:${CRYPTIX_WERROR}>:-Werror>)

# --------------------------------------------------------------------------------
# Generated files
# --------------------------------------------------------------------------------
execute_process(COMMAND git rev-parse HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE git_tag OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT git_tag)
  set(git_tag "unknown")
endif()

# Not __DATE__/__TIME__: a module interface that embeds them is not reproducible.
if(DEFINED ENV{SOURCE_DATE_EPOCH})
  string(TIMESTAMP BUILD_DATE "%b %d %Y" UTC)
else()
  string(TIMESTAMP BUILD_DATE "%b %d %Y")
endif()
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

set(kernel_name "cryptix")
set(CTOS_VERSION_MAJOR ${CryptixOS_VERSION_MAJOR})
set(CTOS_VERSION_MINOR ${CryptixOS_VERSION_MINOR})
set(CTOS_VERSION_PATCH ${CryptixOS_VERSION_PATCH})
set(arch "${CRYPTIX_ARCH}")
set(compiler "${CMAKE_CXX_COMPILER_ID}")
set(compiler_version "${CMAKE_CXX_COMPILER_VERSION}")
set(kernel_version_string
  "${CTOS_VERSION_MAJOR}.${CTOS_VERSION_MINOR}.${CTOS_VERSION_PATCH}-${git_tag}")

# `Version` is a module partition, generated next to the build tree.
configure_file(Version.cppm.in ${CMAKE_CURRENT_BINARY_DIR}/Version.cppm @ONLY)

set(KERNEL_VIRTUAL_RANGE_BASE "0xffff'0000'0000'0000zu")
set(KERNEL_VIRTUAL_RANGE_TOP  "0x6a9e'7f5c'8000'0000zu")
set(USER_VIRTUAL_RANGE_BASE   "0x0000'0000'0010'0000zu")
set(USER_VIRTUAL_RANGE_TOP    "0x0000'7fff'ffff'ffffzu")
set(CTOS_ACPI_DISABLE         0)
set(CTOS_DEVICE_TREE_DISABLE  0)
configure_file(Config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Config.hpp @ONLY)

# Debug switches: 1 enables the corresponding dump / trace.
set(CTOS_MM_DUMP_MEMMAP         0)
set(CTOS_PCI_DUMP_IRQ_ROUTES    0)
set(CTOS_DTB_DUMP               0)
set(CTOS_EFI_DUMP_MEMMAP        0)
set(CTOS_TTY_LOG_TERMIOS        0)
set(CTOS_VFS_DEBUG_ECHFS        0)
set(CTOS_DUMP_INIT_ARRAY        1)
set(CTOS_ELF_DEBUG              0)
set(CTOS_DEVICE_TREE_DUMP       0)
configure_file(Debug/Debug.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Debug/Debug.hpp @ONLY)

# --------------------------------------------------------------------------------
# Module sources
# --------------------------------------------------------------------------------
target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    FILES
      Cryptix.cppm
      ${CMAKE_CURRENT_BINARY_DIR}/Version.cppm
      KernelStart.cpp
      Network/NetworkAdapter.cpp)

foreach(dir API Arch Boot Debug Drivers Embed Firmware Library Memory Network
            Scheduler System Time VFS)
  add_subdirectory(${dir})
endforeach()

# --------------------------------------------------------------------------------
# The kernel image
# --------------------------------------------------------------------------------
add_executable(Cryptix.elf)
target_link_libraries(Cryptix.elf PRIVATE
  cryptix_kernel prism uacpi demangler compiler_rt_builtins)

# Modules that are compiled into the kernel rather than loaded at run time.
set(CRYPTIX_BUILTIN_MODULES "" CACHE INTERNAL "")

set(CRYPTIX_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.${CRYPTIX_ARCH}.ld)
target_link_options(Cryptix.elf PRIVATE
  -nostdlib -static -fuse-ld=lld
  -Wl,-pie -Wl,-z,max-page-size=0x1000
  -Wl,-T,${CRYPTIX_LINKER_SCRIPT}
  $<$<STREQUAL:${CRYPTIX_ARCH},x86_64>:-Wl,-z,noexecstack>
  -Wl,--no-undefined)
set_target_properties(Cryptix.elf PROPERTIES
  LINK_DEPENDS ${CRYPTIX_LINKER_SCRIPT}
  SUFFIX "")
