#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Third-party code from subprojects/, which meson downloads through the
# .wrap files. Run `meson subprojects download` once if subprojects/ is empty.
set(CTOS_SUBPROJECTS_DIR ${CMAKE_SOURCE_DIR}/subprojects)

if(CTOS_ARCH STREQUAL "x86")
    set(CTOS_FREESTND_CXX_ARCH i686)
else()
    set(CTOS_FREESTND_CXX_ARCH ${CTOS_ARCH})
endif()

# Include directories of every dependency, the counterpart of meson's inc_deps.
set(CTOS_DEPENDENCY_INCLUDES
    ${CTOS_SUBPROJECTS_DIR}/demangler/include
    ${CTOS_SUBPROJECTS_DIR}/fmt-10.2.0/include
    ${CTOS_SUBPROJECTS_DIR}/freestnd-cxx-hdrs/${CTOS_FREESTND_CXX_ARCH}/include
    ${CTOS_SUBPROJECTS_DIR}/limine
    ${CTOS_SUBPROJECTS_DIR}/magic_enum/include
    ${CTOS_SUBPROJECTS_DIR}/string/include
    ${CTOS_SUBPROJECTS_DIR}/uacpi/include
)
foreach(dir ${CTOS_DEPENDENCY_INCLUDES})
    if(NOT IS_DIRECTORY ${dir})
        message(FATAL_ERROR "${dir} is missing, run `meson subprojects download`")
    endif()
endforeach()

# Dependencies that meson declares with sources: they are compiled as part of
# the kernel, with the kernel's flags.
set(CTOS_DEPENDENCY_SOURCES
    ${CTOS_SUBPROJECTS_DIR}/demangler/source/demangler.cpp
)
foreach(src
    default_handlers event interpreter io mutex namespace notify opcodes
    opregion osi registers resources shareable sleep stdlib tables types
    uacpi utilities
)
    list(APPEND CTOS_DEPENDENCY_SOURCES ${CTOS_SUBPROJECTS_DIR}/uacpi/source/${src}.c)
endforeach()

# ==============================================================================
### compiler-rt builtins, a static library linked into the kernel. The file
### lists are read from the subproject's meson.build, so they stay in sync.
set(CTOS_COMPILER_RT_DIR ${CTOS_SUBPROJECTS_DIR}/compiler_rt_builtins)

file(READ ${CTOS_COMPILER_RT_DIR}/meson.build CTOS_COMPILER_RT_MESON)
string(REGEX REPLACE "#[^\n]*" "" CTOS_COMPILER_RT_MESON "${CTOS_COMPILER_RT_MESON}")

# Appends the files of the `group = files(...)` list in compiler-rt's
# meson.build to `out`.
function(ctos_compiler_rt_files out group)
    if(NOT CTOS_COMPILER_RT_MESON MATCHES "\n${group} = files\\(([^)]*)\\)")
        message(FATAL_ERROR "compiler-rt: no '${group}' file list in meson.build")
    endif()
    string(REGEX MATCHALL "'[^']+'" files "${CMAKE_MATCH_1}")
    list(TRANSFORM files REPLACE "'" "")
    list(TRANSFORM files PREPEND ${CTOS_COMPILER_RT_DIR}/)
    set(${out} ${${out}} ${files} PARENT_SCOPE)
endfunction()

# Atomics and cpu_model are excluded, as with the subproject's default options,
# and nonbaremetal_files are only for native builds.
set(CTOS_COMPILER_RT_SOURCES)
ctos_compiler_rt_files(CTOS_COMPILER_RT_SOURCES generic_sources)
if(CTOS_ARCH STREQUAL "x86_64")
    foreach(group x86_files x86_64_files generic_nonarm_sources)
        ctos_compiler_rt_files(CTOS_COMPILER_RT_SOURCES ${group})
    endforeach()
elseif(CTOS_ARCH STREQUAL "aarch64")
    foreach(group aarch64_files generic_tf_sources generic_nonx86_64_sources
            generic_nonarm_sources)
        ctos_compiler_rt_files(CTOS_COMPILER_RT_SOURCES ${group})
    endforeach()
endif()

add_library(compiler_rt_builtins STATIC ${CTOS_COMPILER_RT_SOURCES})
set_target_properties(compiler_rt_builtins PROPERTIES C_STANDARD 11)
target_include_directories(compiler_rt_builtins PRIVATE
    ${CMAKE_SOURCE_DIR}/Kernel/Library/KLibC
)

set(CTOS_COMPILER_RT_ARGS
    -fno-builtin
    -fvisibility=hidden
    -fomit-frame-pointer
    -ffreestanding
    -Wno-pedantic
    -Wno-undef
    -Wno-sign-conversion
    -Wno-double-promotion
    -Wno-float-equal
    -Wno-float-conversion
    -Wno-conversion
    -Wno-missing-noreturn
    -Wno-unused-parameter
    -Wno-format-nonliteral
    -Wno-unused-macros
)
foreach(arg
    -Wno-builtin-declaration-mismatch
    -Wno-shift-sign-overflow
    -Wno-shorten-64-to-32
    -Wno-unreachable-code-break
    -Wno-conditional-uninitialized
    -Wno-missing-variable-declarations
    -Wno-reserved-id-macro
)
    string(MAKE_C_IDENTIFIER "CTOS_HAS${arg}" var)
    check_cxx_compiler_flag(${arg} ${var})
    if(${var})
        list(APPEND CTOS_COMPILER_RT_ARGS ${arg})
    endif()
endforeach()
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    check_cxx_compiler_flag(-Wno-missing-prototypes CTOS_HAS_WNO_MISSING_PROTOTYPES)
    if(CTOS_HAS_WNO_MISSING_PROTOTYPES)
        list(APPEND CTOS_COMPILER_RT_ARGS -Wno-missing-prototypes)
    endif()
endif()

# -fPIC: meson builds static libraries with b_staticpic=true.
target_compile_options(compiler_rt_builtins PRIVATE -fPIC ${CTOS_COMPILER_RT_ARGS})
target_compile_definitions(compiler_rt_builtins PRIVATE __ELF__)
