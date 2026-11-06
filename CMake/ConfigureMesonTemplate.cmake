#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# ctos_configure_meson_template(<input> <output>)
#
# Generates <output> from a template written for meson's configure_file(), so
# the same .in files work with both builds:
#  - `#mesondefine NAME` becomes `#define NAME <value of NAME>`, or
#    `/* #undef NAME */` when the CMake variable NAME isn't defined
#  - `@NAME@` is replaced by the value of NAME
# The output is only rewritten when its content changes.
function(ctos_configure_meson_template input output)
    file(READ ${input} content)
    string(REGEX MATCHALL "#mesondefine [A-Za-z0-9_]+" defines "${content}")
    foreach(define ${defines})
        string(REPLACE "#mesondefine " "" name "${define}")
        if(DEFINED ${name})
            set(replacement "#define ${name} ${${name}}")
        else()
            set(replacement "/* #undef ${name} */")
        endif()
        string(REGEX REPLACE "${define}([\r\n])" "${replacement}\\1" content "${content}")
    endforeach()
    file(CONFIGURE OUTPUT ${output} CONTENT "${content}" @ONLY)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${input})
endfunction()
