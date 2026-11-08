#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Host unit tests, built with the host compiler and run with CTest
# (Meta/run-tests.sh does both):
#
#   cmake -S Tests -B build_tests -G Ninja
#   cmake --build build_tests
#   ctest --test-dir build_tests --output-on-failure
#
# - Kernel/: kernel code that doesn't need a running kernel, compiled for the
#   host against Prism, with the headers in Kernel/Shims standing in for the
#   kernel's logger and assertions. Written with GoogleTest.
# - Prism's own suite (Prism/Tests): containers, allocators, strings, paths.
#   Most of its tests are plain assert() programs; CTest runs each one.
cmake_minimum_required(VERSION 3.25)

project(CryptixTests
    VERSION 0.0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CTOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CTOS_SUBPROJECTS_DIR ${CTOS_ROOT}/subprojects)

option(CTOS_TESTS_SANITIZE "Build the tests with ASan and UBSan" ON)
option(CTOS_TESTS_PRISM_SUITE "Also run Prism's own test suite" ON)

if(CTOS_TESTS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

enable_testing()

# ==============================================================================
### GoogleTest, pinned. An installed GTest of at least this version is used
### instead when find_package() finds one.
include(FetchContent)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    FIND_PACKAGE_ARGS 1.17.0 NAMES GTest
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# ==============================================================================
### Prism for the host, from the kernel's checkout of it. Its third-party
### headers come from subprojects/, like the kernel's CMake build.
foreach(dir demangler/include fmt-10.2.0/include magic_enum/include)
    if(NOT IS_DIRECTORY ${CTOS_SUBPROJECTS_DIR}/${dir})
        message(FATAL_ERROR "subprojects/${dir} is missing, "
            "run `meson subprojects download` in the repository root")
    endif()
endforeach()

# The source list is read from Prism/Source/meson.build, so it stays in sync
set(PRISM_SOURCE_DIR ${CTOS_ROOT}/Prism/Source)
file(READ ${PRISM_SOURCE_DIR}/meson.build PRISM_MESON)
if(NOT PRISM_MESON MATCHES "srcs = files\\(([^)]*)\\)")
    message(FATAL_ERROR "Prism: no `srcs = files(...)` list in Source/meson.build")
endif()
string(REGEX MATCHALL "'[^']+'" PRISM_SOURCES "${CMAKE_MATCH_1}")
list(TRANSFORM PRISM_SOURCES REPLACE "'" "")
list(TRANSFORM PRISM_SOURCES PREPEND ${PRISM_SOURCE_DIR}/)

add_library(PrismHost STATIC
    ${PRISM_SOURCES}
    ${CTOS_SUBPROJECTS_DIR}/demangler/source/demangler.cpp
)
target_include_directories(PrismHost PUBLIC
    ${PRISM_SOURCE_DIR}
    ${CTOS_SUBPROJECTS_DIR}/demangler/include
    ${CTOS_SUBPROJECTS_DIR}/fmt-10.2.0/include
    ${CTOS_SUBPROJECTS_DIR}/magic_enum/include
)
# The same configuration as Prism's meson.build for the 'app' target, plus
# PRISM_USE_NAMESPACE, which the kernel and Prism's tests build with
target_compile_definitions(PrismHost PUBLIC
    FMT_HEADER_ONLY=1
    FMT_STATIC_THOUSANDS_SEPARATOR=1
    FMT_USE_LONG_DOUBLE=0
    FMT_USE_DOUBLE=0
    FMT_USE_FLOAT=0
    FMT_USE_FLOAT128=0
    FMT_USE_EXCEPTIONS=0
    MAGIC_ENUM_RANGE_MAX=0x8a00
    PRISM_LOG_ENABLE=1
    PRISM_USE_NAMESPACE=1
)
target_compile_options(PrismHost PUBLIC
    -fno-exceptions
    -fno-rtti
    -fsized-deallocation
    -Wno-unused-parameter
    $<$<CXX_COMPILER_ID:GNU>:-Wno-changes-meaning>
)

# ==============================================================================
### Kernel code on the host
add_library(KernelHost INTERFACE)
# The shims come first so they win over the real kernel headers
target_include_directories(KernelHost INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Kernel/Shims
    ${CTOS_ROOT}/Kernel
)
target_link_libraries(KernelHost INTERFACE PrismHost)

# ctos_kernel_test(<name> SOURCES <test sources> KERNEL <kernel sources>)
#   A GoogleTest program built from the test sources and the kernel sources
#   it covers (paths relative to Kernel/).
function(ctos_kernel_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;KERNEL")
    list(TRANSFORM arg_KERNEL PREPEND ${CTOS_ROOT}/Kernel/)
    add_executable(${name} ${arg_SOURCES} ${arg_KERNEL})
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    target_link_libraries(${name} PRIVATE KernelHost GTest::gtest_main)
    gtest_discover_tests(${name}
        TEST_PREFIX "Kernel."
        DISCOVERY_MODE PRE_TEST
        PROPERTIES LABELS Kernel
    )
endfunction()

add_subdirectory(Kernel)

# ==============================================================================
### Prism's own suite: the tests its meson.build enables, one CTest test each
if(CTOS_TESTS_PRISM_SUITE)
    set(PRISM_TESTS_DIR ${CTOS_ROOT}/Prism/Tests)
    file(GLOB PRISM_TEST_GROUPS LIST_DIRECTORIES true ${PRISM_TESTS_DIR}/*)
    foreach(group_dir ${PRISM_TEST_GROUPS})
        if(NOT EXISTS ${group_dir}/meson.build)
            continue()
        endif()
        get_filename_component(group ${group_dir} NAME)

        # `<group>_tests = [ 'A', 'B', ... ]`, without commented-out entries
        file(READ ${group_dir}/meson.build group_meson)
        string(REGEX REPLACE "#[^\n]*" "" group_meson "${group_meson}")
        if(NOT group_meson MATCHES "_tests = \\[([^]]*)\\]")
            continue()
        endif()
        string(REGEX MATCHALL "'[^']+'" names "${CMAKE_MATCH_1}")
        list(TRANSFORM names REPLACE "'" "")

        foreach(name ${names})
            set(source ${group_dir}/${name}/main.cpp)
            set(target Prism.${group}.${name})
            add_executable(${target} ${source})
            target_compile_options(${target} PRIVATE -Wno-self-assign-overloaded)
            target_link_libraries(${target} PRIVATE PrismHost)

            # A few tests use GoogleTest; the rest are assert() programs
            # with their own main()
            file(READ ${source} test_source)
            if(test_source MATCHES "gtest/gtest.h")
                target_link_libraries(${target} PRIVATE GTest::gtest_main)
                gtest_discover_tests(${target}
                    TEST_PREFIX "${target}."
                    DISCOVERY_MODE PRE_TEST
                    PROPERTIES LABELS Prism
                )
            else()
                # assert() must stay on
                target_compile_options(${target} PRIVATE -UNDEBUG)
                add_test(NAME ${target} COMMAND ${target})
                set_tests_properties(${target} PROPERTIES LABELS Prism)
            endif()
        endforeach()
    endforeach()
endif()
