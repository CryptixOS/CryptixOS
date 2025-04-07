/*
 * Created by v1tr10l7 on 03.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Core/Types.hpp>
#include <Prism/StringView.hpp>

constexpr const char* KERNEL_NAME          = "@kernel_name@";
constexpr usize       KERNEL_VERSION_MAJOR = @kernel_version_major @;
constexpr usize       KERNEL_VERSION_MINOR = @kernel_version_minor @;
constexpr usize       KERNEL_VERSION_PATCH = @kernel_version_patch @;
#define KERNEL_GIT_TAG "@git_tag@";

#ifdef CTOS_TARGET_X86_64
constexpr const char* KERNEL_ARCH_STRING = "x86_64";
#elifdef CTOS_TARGET_AARCH64
constexpr const char* KERNEL_ARCH_STRING = "aarch64";
#elifdef CTOS_TARGET_RISCV
constexpr const char* KERNEL_ARCH_STRING = "riscv";
#endif

constexpr const char* KERNEL_BUILD_DATE = __DATE__;
constexpr const char* KERNEL_BUILD_TIME = __TIME__;
#if (defined(__GNUC__) || defined(__GNUG__)) && !defined(__clang__)
constexpr const char* KERNEL_COMPILER  = "gcc";
constexpr const char* COMPILER_VERSION = __VERSION__;
#elifdef __clang__
constexpr const char* KERNEL_COMPILER  = "clang";
constexpr const char* COMPILER_VERSION = __clang_version__;
#else
constexpr const char* KERNEL_COMPILER  = "unknown";
constexpr const char* COMPILER_VERSION = "?.?.?";
#endif

constexpr StringView KERNEL_VERSION_STRING
    = Stringify(@kernel_version_major @) "." Stringify(
        @kernel_version_minor
        @) "." Stringify(@kernel_version_patch @) "-" KERNEL_GIT_TAG;
