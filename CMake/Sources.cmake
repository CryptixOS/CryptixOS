#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# The CMake counterpart of meson's `srcs += files(...)` and `subdir(...)`:
# every source directory has a Sources.cmake next to its meson.build that
# adds its files and includes its subdirectories. They are include()d, not
# add_subdirectory()d, so like meson's subdir() they all share one scope and
# can append to the same lists.

# ctos_subdir(<dir>)
#   Processes <dir>/Sources.cmake, relative to the calling file.
macro(ctos_subdir dir)
    include(${CMAKE_CURRENT_LIST_DIR}/${dir}/Sources.cmake)
endmacro()

# ctos_kernel_sources(<files>...)
#   Appends <files>, relative to the calling file, to CTOS_KERNEL_SOURCES.
function(ctos_kernel_sources)
    set(files ${ARGN})
    list(TRANSFORM files PREPEND ${CMAKE_CURRENT_LIST_DIR}/)
    set(CTOS_KERNEL_SOURCES ${CTOS_KERNEL_SOURCES} ${files} PARENT_SCOPE)
endfunction()
