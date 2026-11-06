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
    list(APPEND CTOS_MODULE_ARGS
        -DCTOS_ARCH=CTOS_ARCH_X86_64 -DCTOS_TARGET_X86_64
        -msoft-float
        -mno-mmx
        -mno-sse
        -mno-sse2
        -mno-red-zone
        -mcmodel=large
    )
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
function(ctos_add_module name kind)
    set(sources ${ARGN})
    list(TRANSFORM sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
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

# Same modules as the meson.build files under Modules/.
if(CTOS_ARCH STREQUAL "x86_64")
    ctos_add_module(i8042 EXTERNAL x86_64/input/i8042/main.cpp)
    ctos_add_module(pcspk EXTERNAL x86_64/pcspk/main.cpp)
endif()
ctos_add_module(e1000e EXTERNAL noarch/net/e1000e/main.cpp)
ctos_add_module(rtl8139 BUILTIN noarch/net/rtl8139/main.cpp)
ctos_add_module(serial_pnp EXTERNAL serial/serial_pnp/main.cpp)
ctos_add_module(serio EXTERNAL serio/main.cpp)
ctos_add_module(atkbd EXTERNAL input/atkbd/main.cpp)

set(CTOS_BUILTIN_MODULE_SOURCES ${CTOS_BUILTIN_MODULE_SOURCES} PARENT_SCOPE)
set(CTOS_EXTERNAL_MODULES ${CTOS_EXTERNAL_MODULES} PARENT_SCOPE)
