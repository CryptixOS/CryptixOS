/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

//--------------------------------------------------------------------------
// Compiler attributes
//--------------------------------------------------------------------------

#define CTOS_SECTION(name, ...)                                                \
    [[CTOS_ATTR_PREFIX::section(name) __VA_OPT__(, ) __VA_ARGS__]]

#ifdef __clang__
    #define CTOS_ATTR_PREFIX gnu
    #define CTOS_COMPILER_CLANG
    #define CTOS_COMPILER_NAME_STR "clang"
#elifdef __GNUC__
    #define CTOS_ATTR_PREFIX gnu
    #define CTOS_COMPILER_GCC
    #define CTOS_COMPILER_NAME_STR "gcc"
#else
    #error "Unsupported compiler"
#endif

#define CTOS_ATTRIBUTE(name)           [[CTOS_ATTR_PREFIX::name]]
#define CTOS_ALWAYS_INLINE             CTOS_ATTRIBUTE(always_inline) inline
#define CTOS_UNUSED                    [[maybe_unused]]
#define CTOS_FORCE_EMIT                [[CTOS_ATTR_PREFIX::used]]
#define CTOS_FALLTHROUGH               [[fallthrough]]
#define CTOS_NORETURN                  [[noreturn]]
#define CTOS_NODISCARD                 [[nodiscard]]
#define CTOS_WEAK                      [[weak]]
#define CTOS_LIKELY                    [[likely]]
#define CTOS_UNLIKELY                  [[unlikely]]
#define CTOS_PACKED                    [[gnu::packed]]
#define CTOS_PACKED_ALIGNED(alignment) [[gnu::packed, gnu::aligned(alignment)]]

#define CTOS_NO_SANITIZE(name)         [[clang::no_sanitize(name)]]

#define CTOS_SECTION_FORCE_EMIT(name, ...)                                     \
    CTOS_SECTION(name, CTOS_ATTR_PREFIX::used __VA_OPT__(, ) __VA_ARGS__)

#define CTOS_CDECL               CTOS_ATTRIBUTE(cdecl)
#define CTOS_FASTCALL            CTOS_ATTRIBUTE(rastcall)
#define CTOS_SYSVABI             CTOS_ATTRIBUTE(sysv_abi)
#define CTOS_NAKED               CTOS_ATTRIBUTE(naked)

#define CTOS_EXPORT              __attribute__((visibility("default")))

#define CtStringifyInner(x)      #x
#define CtStringify(x)           CtStringifyInner(x)

#define CtConcatenateName(a, b)  CtConcatenateInner(a, b)
#define CtConcatenateInner(a, b) a##b
#define CtUniqueName(name)                                                     \
    CtConcatenateName(_##name##__, CtConcatenateName(__COUNTER__, __LINE__))

//--------------------------------------------------------------------------
// Sections
//--------------------------------------------------------------------------

#define KERNEL_INIT_SECTION_NAME ".kernel_init"
#define KERNEL_INIT_SECTION      CTOS_SECTION_FORCE_EMIT(KERNEL_INIT_SECTION_NAME)

#define MODULE_SECTION_NAME      ".module_init"
#define MODULE_SECTION           CTOS_SECTION_FORCE_EMIT(MODULE_SECTION_NAME)

#define MODULE_DATA_SECTION_NAME ".module_init.data"

//--------------------------------------------------------------------------
// Compiler builtins
//--------------------------------------------------------------------------

// NOTE(v1tr10l7): index must be a value between 0 - 63
#define CtFrameAddress(index)    __builtin_frame_address(index)
#define CtCurrentFrameAddress()  CtFrameAddress(0)
