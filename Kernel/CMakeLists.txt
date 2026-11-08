#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
include(${CMAKE_SOURCE_DIR}/CMake/ConfigureMesonTemplate.cmake)

# ==============================================================================
### Prism, compiled into the kernel (meson.build's subdir('Prism/Source'))
set(CTOS_PRISM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Prism/Source)
set(CTOS_PRISM_SOURCES)
foreach(src
    Algorithm/Hash.cpp

    Containers/Bitmap.cpp
    Containers/RingBuffer.cpp

    Debug/Assertions.cpp
    Debug/Log.cpp
    Debug/Logger.cpp
    Debug/LogSink.cpp
    Debug/Ubsan.cpp
    Debug/Stacktrace.cpp

    Memory/Memory.cpp
    String/Printf.cpp

    Utility/Path.cpp
    Utility/PathView.cpp
    Utility/Time.cpp
)
    list(APPEND CTOS_PRISM_SOURCES ${CTOS_PRISM_SOURCE_DIR}/Prism/${src})
endforeach()

# ==============================================================================
### Generated headers
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(kernel_name cryptix)
set(CTOS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CTOS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CTOS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(arch ${CTOS_ARCH})
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler)
if(compiler STREQUAL "gnu")
    set(compiler gcc)
endif()
set(compiler_version ${CMAKE_CXX_COMPILER_VERSION})
set(kernel_version_string "${PROJECT_VERSION}-${git_tag}")
ctos_configure_meson_template(Version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Version.hpp)

set(KERNEL_VIRTUAL_RANGE_BASE "0xffff'0000'0000'0000zu")
# Kernel virtual allocations end where the kernel image starts.
set(KERNEL_VIRTUAL_RANGE_TOP "0xffff'ffff'8000'0000zu")
set(USER_VIRTUAL_RANGE_BASE "0x0000'0000'0010'0000zu")
set(USER_VIRTUAL_RANGE_TOP "0x0000'7fff'ffff'ffffzu")
set(CTOS_ACPI_DISABLE 0)
set(CTOS_DEVICE_TREE_DISABLE 0)
ctos_configure_meson_template(Config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Config.hpp)

# ==============================================================================
### Compiler arguments
set(CTOS_KERNEL_C_ARGS)
set(CTOS_KERNEL_CXX_ARGS)
check_cxx_compiler_flag(-Wno-changes-meaning CTOS_HAS_WNO_CHANGES_MEANING)
if(CTOS_HAS_WNO_CHANGES_MEANING)
    list(APPEND CTOS_KERNEL_C_ARGS -Wglobal-constructors)
    list(APPEND CTOS_KERNEL_CXX_ARGS -Wglobal-constructors)
endif()

set(CTOS_KERNEL_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.${CTOS_ARCH}.ld)
set(CTOS_KERNEL_LINK_ARGS
    -nostdlib
    -static
    -pie
    -Wl,--fatal-warnings
    -Wl,--as-needed
    -Wl,--no-undefined
    -Wl,-pie
    -Wl,-z,max-page-size=0x1000
    -Wl,-T,${CTOS_KERNEL_LINKER_SCRIPT}
)

if(CTOS_ARCH STREQUAL "x86_64")
    list(APPEND CTOS_KERNEL_CXX_ARGS -DCTOS_ARCH=CTOS_ARCH_X86_64 -DCTOS_TARGET_X86_64)
elseif(CTOS_ARCH STREQUAL "aarch64")
    list(APPEND CTOS_KERNEL_CXX_ARGS -DCTOS_ARCH=CTOS_ARCH_AARCH64 -DCTOS_TARGET_AARCH64)
endif()

# KASAN (Debug/Kasan.hpp): every access calls into the runtime, which has no
# fixed shadow offset to inline checks against. Stack instrumentation would
# write the shadow inline, so it stays off. The kernel only, not the .ko
# modules, which have no runtime to call.
if(CTOS_KASAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "CTOS_KASAN needs clang")
    endif()
    set(kasan_args
        -fsanitize=kernel-address
        -mllvm=-asan-instrumentation-with-call-threshold=0
        -mllvm=-asan-stack=0
        -mllvm=-asan-globals=1
        -DCTOS_KASAN=1
    )
    list(APPEND CTOS_KERNEL_C_ARGS ${kasan_args})
    list(APPEND CTOS_KERNEL_CXX_ARGS ${kasan_args})
endif()

set(CTOS_KERNEL_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ==============================================================================
### Sources: every directory's Sources.cmake (see CMake/Sources.cmake), in
### the same order as the meson.build files, so the link order (and with it
### the order of global constructors) is the same. They also add the
### architecture's compiler arguments and Library/KLibC's include paths.
set(CTOS_KERNEL_SOURCES)
ctos_kernel_sources(
    KernelStart.cpp

    Network/NetworkAdapter.cpp
)

ctos_subdir(API)
ctos_subdir(Arch)
ctos_subdir(Boot)
ctos_subdir(Debug)
ctos_subdir(Drivers)
ctos_subdir(Firmware)
ctos_subdir(Library)
ctos_subdir(Memory)
ctos_subdir(Network)
ctos_subdir(Scheduler)
ctos_subdir(System)
ctos_subdir(Time)
ctos_subdir(VFS)

list(APPEND CTOS_KERNEL_INCLUDES
    ${CTOS_PRISM_SOURCE_DIR}
    ${CTOS_DEPENDENCY_INCLUDES}
)

# ==============================================================================
### C++ modules. Their interface units are compiled once, with the kernel's
### flags, and imported by the kernel and by the .ko modules.
add_library(KernelModules OBJECT)
target_sources(KernelModules
    PUBLIC FILE_SET CXX_MODULES FILES
        Arch/PowerManager.ixx
)
target_include_directories(KernelModules PRIVATE ${CTOS_KERNEL_INCLUDES})
target_compile_options(KernelModules PRIVATE -fPIE ${CTOS_KERNEL_CXX_ARGS})

# ==============================================================================
### Cryptix.elf
add_executable(Kernel
    ${CTOS_PRISM_SOURCES}
    ${CTOS_KERNEL_SOURCES}
    ${CTOS_DEPENDENCY_SOURCES}
    ${CTOS_BUILTIN_MODULE_SOURCES}
)
set_target_properties(Kernel PROPERTIES
    OUTPUT_NAME Cryptix.elf
    SUFFIX ""
    LINK_DEPENDS ${CTOS_KERNEL_LINKER_SCRIPT}
)
target_include_directories(Kernel PRIVATE ${CTOS_KERNEL_INCLUDES})
target_compile_options(Kernel PRIVATE
    -fPIE
    "$<$<COMPILE_LANGUAGE:C,ASM>:${CTOS_KERNEL_C_ARGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${CTOS_KERNEL_CXX_ARGS}>"
)
target_precompile_headers(Kernel PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Library/PrecompiledHeader.hpp>"
)
target_link_options(Kernel PRIVATE ${CTOS_KERNEL_LINK_ARGS})
target_link_libraries(Kernel PRIVATE KernelModules compiler_rt_builtins)
