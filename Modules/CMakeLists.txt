#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Kernel modules. A module is either built into the kernel (its sources are
# compiled as part of Cryptix.elf) or external: compiled on its own and linked
# into <name>.ko, which the kernel loads at runtime from /usr/lib/modules.
set(CTOS_MODULE_ARGS
    -fsanitize=undefined
    -fmacro-backtrace-limit=0
    -fPIC
)
if(CTOS_ARCH STREQUAL "x86_64")
    # The rest of the x86_64 arguments are added by x86_64/Sources.cmake
    list(APPEND CTOS_MODULE_ARGS -DCTOS_ARCH=CTOS_ARCH_X86_64 -DCTOS_TARGET_X86_64)
elseif(CTOS_ARCH STREQUAL "aarch64")
    list(APPEND CTOS_MODULE_ARGS
        -DCTOS_ARCH=CTOS_ARCH_AARCH64 -DCTOS_TARGET_AARCH64
        -mgeneral-regs-only
        -march=armv8-a+nofp+nosimd
        -mcmodel=small
    )
endif()

set(CTOS_MODULE_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
set(CTOS_MODULE_LINK_ARGS -znoexecstack -shared)
find_program(CTOS_LD_LLD ld.lld REQUIRED)

set(CTOS_BUILTIN_MODULE_SOURCES)
set(CTOS_EXTERNAL_MODULES)

# ctos_add_module(<name> BUILTIN|EXTERNAL <sources>...)
#   Sources are relative to the calling file, a module's Sources.cmake.
function(ctos_add_module name kind)
    set(sources ${ARGN})
    list(TRANSFORM sources PREPEND ${CMAKE_CURRENT_LIST_DIR}/)
    if(kind STREQUAL "BUILTIN")
        set(CTOS_BUILTIN_MODULE_SOURCES ${CTOS_BUILTIN_MODULE_SOURCES} ${sources}
            PARENT_SCOPE)
        return()
    endif()

    list(GET sources 0 first_source)
    get_filename_component(module_dir ${first_source} DIRECTORY)

    set(objects ${name}_objects)
    add_library(${objects} OBJECT ${sources})
    target_include_directories(${objects} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Kernel
        ${CMAKE_BINARY_DIR}/Kernel
        ${CMAKE_SOURCE_DIR}/Kernel/Library/KLibC
        ${CMAKE_SOURCE_DIR}/Kernel/Library/KLibC++
        ${CMAKE_SOURCE_DIR}/Prism/Source
        ${CTOS_DEPENDENCY_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${module_dir}
    )
    target_compile_options(${objects} PRIVATE ${CTOS_MODULE_ARGS})
    # For the kernel's module interfaces (BMIs) only: an object library linked
    # to another one doesn't take its objects, the kernel provides the symbols.
    target_link_libraries(${objects} PRIVATE KernelModules)

    set(ko ${CMAKE_BINARY_DIR}/Modules/${name}.ko)
    add_custom_command(
        OUTPUT ${ko}
        COMMAND ${CTOS_LD_LLD} ${CTOS_MODULE_LINK_ARGS} -T ${CTOS_MODULE_LINKER_SCRIPT}
                "$<TARGET_OBJECTS:${objects}>" -o ${ko}
        DEPENDS ${objects} "$<TARGET_OBJECTS:${objects}>" ${CTOS_MODULE_LINKER_SCRIPT}
        COMMAND_EXPAND_LISTS
        COMMENT "Linking kernel module ${name}.ko"
        VERBATIM
    )
    add_custom_target(${name}_ko ALL DEPENDS ${ko})
    set(CTOS_EXTERNAL_MODULES ${CTOS_EXTERNAL_MODULES} ${ko} PARENT_SCOPE)
endfunction()

# Every module declares itself in its directory's Sources.cmake, next to its
# meson.build, and the directories are walked in meson.build's order.
ctos_subdir(${CTOS_ARCH})
ctos_subdir(noarch)
ctos_subdir(serial)
ctos_subdir(serio)
ctos_subdir(input)

set(CTOS_BUILTIN_MODULE_SOURCES ${CTOS_BUILTIN_MODULE_SOURCES} PARENT_SCOPE)
set(CTOS_EXTERNAL_MODULES ${CTOS_EXTERNAL_MODULES} PARENT_SCOPE)
